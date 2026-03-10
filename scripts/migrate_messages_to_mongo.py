#!/usr/bin/env python3
"""
Backfill MemoChat message history from MySQL into MongoDB.

Dependencies:
  pip install pymysql pymongo

Example:
  python scripts/migrate_messages_to_mongo.py --config server/ChatServer/chatserver1.ini
"""

from __future__ import annotations

import argparse
import configparser
from pathlib import Path
from typing import Dict, Iterable, List

import pymongo
import pymysql
from pymongo import UpdateOne


def load_ini(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    with path.open("r", encoding="utf-8") as fh:
        parser.read_file(fh)
    return parser


def mysql_connect(cfg: configparser.ConfigParser):
    host = cfg.get("Mysql", "Host", fallback="127.0.0.1").replace("tcp://", "")
    return pymysql.connect(
        host=host,
        port=cfg.getint("Mysql", "Port", fallback=3306),
        user=cfg.get("Mysql", "User", fallback="root"),
        password=cfg.get("Mysql", "Passwd", fallback=""),
        database=cfg.get("Mysql", "Schema", fallback="memo"),
        charset="utf8mb4",
        cursorclass=pymysql.cursors.DictCursor,
        autocommit=True,
    )


def mongo_connect(cfg: configparser.ConfigParser):
    uri = cfg.get("Mongo", "Uri")
    db_name = cfg.get("Mongo", "Database", fallback="memochat")
    client = pymongo.MongoClient(uri)
    return client, client[db_name]


def iter_private_rows(conn, batch_size: int) -> Iterable[List[Dict]]:
    sql = """
        SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content,
               reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at
        FROM chat_private_msg
        ORDER BY created_at ASC, msg_id ASC
        LIMIT %s OFFSET %s
    """
    offset = 0
    while True:
        with conn.cursor() as cur:
            cur.execute(sql, (batch_size, offset))
            rows = cur.fetchall()
        if not rows:
            break
        yield rows
        offset += len(rows)


def iter_group_rows(conn, batch_size: int) -> Iterable[List[Dict]]:
    sql = """
        SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content,
               m.mentions_json, m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms,
               m.deleted_at_ms, m.created_at, COALESCE(e.file_name, '') AS file_name,
               COALESCE(e.mime, '') AS mime, COALESCE(e.size, 0) AS size,
               COALESCE(u.name, '') AS from_name, COALESCE(u.nick, '') AS from_nick,
               COALESCE(u.icon, '') AS from_icon
        FROM chat_group_msg m
        LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id
        LEFT JOIN user u ON m.from_uid = u.uid
        ORDER BY m.group_id ASC, m.group_seq ASC, m.server_msg_id ASC
        LIMIT %s OFFSET %s
    """
    offset = 0
    while True:
        with conn.cursor() as cur:
            cur.execute(sql, (batch_size, offset))
            rows = cur.fetchall()
        if not rows:
            break
        yield rows
        offset += len(rows)


def private_doc(row: Dict) -> Dict:
    return {
        "_id": row["msg_id"],
        "msg_id": row["msg_id"],
        "conv_key": f"p_{min(row['conv_uid_min'], row['conv_uid_max'])}_{max(row['conv_uid_min'], row['conv_uid_max'])}",
        "conv_uid_min": int(row["conv_uid_min"]),
        "conv_uid_max": int(row["conv_uid_max"]),
        "from_uid": int(row["from_uid"]),
        "to_uid": int(row["to_uid"]),
        "content": row.get("content", "") or "",
        "reply_to_server_msg_id": int(row.get("reply_to_server_msg_id") or 0),
        "forward_meta_json": row.get("forward_meta_json", "") or "",
        "edited_at_ms": int(row.get("edited_at_ms") or 0),
        "deleted_at_ms": int(row.get("deleted_at_ms") or 0),
        "created_at": int(row.get("created_at") or 0),
    }


def group_doc(row: Dict) -> Dict:
    return {
        "_id": row["msg_id"],
        "msg_id": row["msg_id"],
        "group_id": int(row["group_id"]),
        "server_msg_id": int(row.get("server_msg_id") or 0),
        "group_seq": int(row.get("group_seq") or 0),
        "from_uid": int(row["from_uid"]),
        "msg_type": row.get("msg_type", "text") or "text",
        "content": row.get("content", "") or "",
        "mentions_json": row.get("mentions_json", "[]") or "[]",
        "file_name": row.get("file_name", "") or "",
        "mime": row.get("mime", "") or "",
        "size": int(row.get("size") or 0),
        "reply_to_server_msg_id": int(row.get("reply_to_server_msg_id") or 0),
        "forward_meta_json": row.get("forward_meta_json", "") or "",
        "edited_at_ms": int(row.get("edited_at_ms") or 0),
        "deleted_at_ms": int(row.get("deleted_at_ms") or 0),
        "created_at": int(row.get("created_at") or 0),
        "from_name": row.get("from_name", "") or "",
        "from_nick": row.get("from_nick", "") or "",
        "from_icon": row.get("from_icon", "") or "",
    }


def bulk_upsert(collection, docs: List[Dict]) -> int:
    if not docs:
        return 0
    ops = [UpdateOne({"_id": doc["_id"]}, {"$set": doc}, upsert=True) for doc in docs]
    result = collection.bulk_write(ops, ordered=False)
    return result.upserted_count + result.modified_count


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default="server/ChatServer/chatserver1.ini")
    parser.add_argument("--batch-size", type=int, default=500)
    parser.add_argument("--skip-private", action="store_true")
    parser.add_argument("--skip-group", action="store_true")
    args = parser.parse_args()

    cfg = load_ini(Path(args.config))
    mysql_conn = mysql_connect(cfg)
    mongo_client, mongo_db = mongo_connect(cfg)
    private_collection = mongo_db[cfg.get("Mongo", "PrivateCollection", fallback="private_messages")]
    group_collection = mongo_db[cfg.get("Mongo", "GroupCollection", fallback="group_messages")]

    migrated_private = 0
    migrated_group = 0
    try:
        if not args.skip_private:
            for rows in iter_private_rows(mysql_conn, args.batch_size):
                migrated_private += bulk_upsert(private_collection, [private_doc(row) for row in rows])
                print(f"[private] processed={migrated_private}")

        if not args.skip_group:
            for rows in iter_group_rows(mysql_conn, args.batch_size):
                migrated_group += bulk_upsert(group_collection, [group_doc(row) for row in rows])
                print(f"[group] processed={migrated_group}")
    finally:
        mysql_conn.close()
        mongo_client.close()

    print(f"done private={migrated_private} group={migrated_group}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
