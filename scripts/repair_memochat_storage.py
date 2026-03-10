#!/usr/bin/env python3
import argparse
import configparser
import json
from pathlib import Path

import pymysql


def get_value(row, key):
    if row is None:
        return None
    if key in row:
        return row[key]
    upper = key.upper()
    if upper in row:
        return row[upper]
    lower = key.lower()
    if lower in row:
        return row[lower]
    raise KeyError(key)


def load_config(config_path: Path):
    cfg = configparser.ConfigParser()
    cfg.read(config_path, encoding="utf-8")
    return {
        "host": cfg.get("Mysql", "Host").replace("tcp://", ""),
        "port": cfg.getint("Mysql", "Port"),
        "user": cfg.get("Mysql", "User"),
        "password": cfg.get("Mysql", "Passwd"),
        "database": cfg.get("Mysql", "Schema"),
    }


def connect_mysql(db_cfg):
    return pymysql.connect(
        host=db_cfg["host"],
        port=db_cfg["port"],
        user=db_cfg["user"],
        password=db_cfg["password"],
        database=db_cfg["database"],
        charset="utf8mb4",
        autocommit=False,
        cursorclass=pymysql.cursors.DictCursor,
    )


def fetch_one(cur, sql, params=None):
    cur.execute(sql, params or ())
    return cur.fetchone()


def fetch_all(cur, sql, params=None):
    cur.execute(sql, params or ())
    return cur.fetchall()


def index_exists(cur, schema, table, index_name):
    row = fetch_one(
        cur,
        """
        SELECT COUNT(*) AS cnt
        FROM information_schema.statistics
        WHERE table_schema=%s AND table_name=%s AND index_name=%s
        """,
        (schema, table, index_name),
    )
    return bool(row and int(get_value(row, "cnt")) > 0)


def count_indexes_for_column(cur, schema, table, column_name):
    row = fetch_one(
        cur,
        """
        SELECT COUNT(DISTINCT index_name) AS cnt
        FROM information_schema.statistics
        WHERE table_schema=%s AND table_name=%s AND column_name=%s
        """,
        (schema, table, column_name),
    )
    return int(get_value(row, "cnt")) if row else 0


def drop_index_if_exists(cur, table, index_name):
    cur.execute(
        """
        SELECT COUNT(*) AS cnt
        FROM information_schema.statistics
        WHERE table_schema = DATABASE() AND table_name=%s AND index_name=%s
        """,
        (table, index_name),
    )
    row = cur.fetchone()
    if row and int(get_value(row, "cnt")) > 0:
        cur.execute(f"DROP INDEX {index_name} ON {table}")


def collect_stats(cur):
    duplicate_group_seq = fetch_one(
        cur,
        """
        SELECT COUNT(*) AS cnt
        FROM (
            SELECT group_id, group_seq
            FROM chat_group_msg
            GROUP BY group_id, group_seq
            HAVING COUNT(*) > 1
        ) AS dup
        """,
    )
    dialog_indexes = fetch_all(
        cur,
        """
        SELECT DISTINCT index_name
        FROM information_schema.statistics
        WHERE table_schema = DATABASE()
          AND table_name='chat_dialog'
          AND column_name IN ('owner_uid', 'dialog_type', 'peer_uid', 'group_id')
        ORDER BY index_name
        """,
    )
    group_msg_indexes = fetch_all(
        cur,
        """
        SELECT DISTINCT index_name
        FROM information_schema.statistics
        WHERE table_schema = DATABASE()
          AND table_name='chat_group_msg'
          AND column_name IN ('server_msg_id', 'group_id', 'group_seq')
        ORDER BY index_name
        """,
    )
    return {
        "duplicate_group_seq_pairs": int(get_value(duplicate_group_seq, "cnt")),
        "dialog_indexes": [get_value(row, "index_name") for row in dialog_indexes],
        "group_msg_indexes": [get_value(row, "index_name") for row in group_msg_indexes],
    }


def rebuild_group_seq(cur):
    cur.execute(
        """
        CREATE TEMPORARY TABLE IF NOT EXISTS tmp_group_seq_backfill (
            msg_id VARCHAR(64) PRIMARY KEY,
            group_seq BIGINT NOT NULL
        ) ENGINE=InnoDB
        """
    )
    cur.execute("TRUNCATE TABLE tmp_group_seq_backfill")
    cur.execute("SET @memochat_prev_gid := -1, @memochat_seq := 0")
    cur.execute(
        """
        INSERT INTO tmp_group_seq_backfill(msg_id, group_seq)
        SELECT ranked.msg_id, ranked.group_seq
        FROM (
            SELECT ordered.msg_id,
                   (@memochat_seq := IF(@memochat_prev_gid = ordered.group_id, @memochat_seq + 1, 1)) AS group_seq,
                   (@memochat_prev_gid := ordered.group_id) AS prev_gid
            FROM (
                SELECT msg_id, group_id
                FROM chat_group_msg
                ORDER BY group_id ASC, created_at ASC, msg_id ASC
            ) AS ordered
        ) AS ranked
        """
    )
    cur.execute(
        """
        UPDATE chat_group_msg m
        JOIN tmp_group_seq_backfill t ON t.msg_id = m.msg_id
        SET m.group_seq = t.group_seq
        """
    )
    cur.execute("DROP TEMPORARY TABLE IF EXISTS tmp_group_seq_backfill")


def normalize_indexes(cur, schema):
    if not index_exists(cur, schema, "chat_dialog", "uq_owner_dialog"):
        cur.execute(
            "CREATE UNIQUE INDEX uq_owner_dialog ON chat_dialog(owner_uid, dialog_type, peer_uid, group_id)"
        )
    if not index_exists(cur, schema, "chat_dialog", "idx_owner_type"):
        cur.execute("CREATE INDEX idx_owner_type ON chat_dialog(owner_uid, dialog_type)")
    drop_index_if_exists(cur, "chat_dialog", "uq_chat_dialog_owner_dialog")
    drop_index_if_exists(cur, "chat_dialog", "idx_chat_dialog_owner_type")

    if not index_exists(cur, schema, "chat_group_msg", "idx_group_seq"):
        cur.execute("CREATE INDEX idx_group_seq ON chat_group_msg(group_id, group_seq)")

    drop_index_if_exists(cur, "chat_group_msg", "idx_chat_group_msg_group_seq")
    if count_indexes_for_column(cur, schema, "chat_group_msg", "server_msg_id") > 1:
        drop_index_if_exists(cur, "chat_group_msg", "uk_chat_group_msg_server_msg_id")
    if not index_exists(cur, schema, "chat_group_msg", "uk_chat_group_msg_group_seq"):
        cur.execute("CREATE UNIQUE INDEX uk_chat_group_msg_group_seq ON chat_group_msg(group_id, group_seq)")


def main():
    parser = argparse.ArgumentParser(description="Repair MemoChat storage drift")
    parser.add_argument(
        "--config",
        default="server/ChatServer/config.ini",
        help="Path to ChatServer config.ini",
    )
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    db_cfg = load_config(config_path)
    conn = connect_mysql(db_cfg)
    try:
        with conn.cursor() as cur:
            before = collect_stats(cur)
            rebuild_group_seq(cur)
            normalize_indexes(cur, db_cfg["database"])
            after = collect_stats(cur)
        conn.commit()
        print(json.dumps({"before": before, "after": after}, ensure_ascii=False, indent=2))
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


if __name__ == "__main__":
    main()
