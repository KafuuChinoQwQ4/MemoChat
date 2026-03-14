import argparse
import time
import uuid

from memochat_load_common import (
    ensure_accounts,
    finalize_report,
    get_log_dir,
    get_postgresql_config,
    get_runtime_accounts_csv,
    init_json_logger,
    load_json,
    open_postgresql,
    refresh_account_profiles,
    run_parallel,
    utc_now_str,
)


def seed_postgresql_data(cfg, owner_uid: int, peer_uid: int) -> int:
    conn = open_postgresql(cfg)
    try:
        group_code = f"g{str(int(time.time()))[-9:]}"
        group_name = f"postgresql-load-{uuid.uuid4().hex[:8]}"
        with conn.cursor() as cursor:
            cursor.execute(
                "INSERT INTO chat_group(group_code, name, owner_uid, announcement, member_limit, status) "
                "VALUES(%s,%s,%s,%s,%s,1) RETURNING group_id",
                [group_code, group_name, owner_uid, "loadtest", 200],
            )
            group_id = int(cursor.fetchone()["group_id"])
            cursor.execute(
                "INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                "VALUES(%s,%s,3,0,1,1) "
                "ON CONFLICT (group_id, uid) DO UPDATE SET status = 1, role = 3, mute_until = 0",
                [group_id, owner_uid],
            )
            cursor.execute(
                "INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                "VALUES(%s,%s,1,0,1,1) "
                "ON CONFLICT (group_id, uid) DO UPDATE SET status = 1, role = 1, mute_until = 0",
                [group_id, peer_uid],
            )
            now_ms = int(time.time() * 1000)
            conv_min = min(owner_uid, peer_uid)
            conv_max = max(owner_uid, peer_uid)
            for idx in range(30):
                cursor.execute(
                    "INSERT INTO chat_private_msg(msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, created_at) "
                    "VALUES(%s,%s,%s,%s,%s,%s,%s)",
                    [f"postgresql-private-{uuid.uuid4().hex}", conv_min, conv_max, owner_uid if idx % 2 == 0 else peer_uid,
                     peer_uid if idx % 2 == 0 else owner_uid, f"seed-private-{idx}", now_ms + idx],
                )
            for idx in range(30):
                group_seq = time.time_ns()
                cursor.execute(
                    "INSERT INTO chat_group_msg(msg_id, group_id, from_uid, msg_type, content, created_at, group_seq) "
                    "VALUES(%s,%s,%s,%s,%s,%s,%s)",
                    [f"postgresql-group-{uuid.uuid4().hex}", group_id, owner_uid if idx % 2 == 0 else peer_uid,
                     "text", f"seed-group-{idx}", now_ms + idx, group_seq],
                )
        conn.commit()
        return group_id
    finally:
        conn.close()


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat PostgreSQL capacity load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total operations")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("postgresql_capacity_loadtest", log_dir=get_log_dir(cfg))
    test_cfg = cfg.get("postgresql_capacity", cfg.get("mysql_capacity", {}))
    total = args.total if args.total > 0 else int(test_cfg.get("total", 500))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 20))
    workload_mix = list(test_cfg.get("workload_mix", ["read", "write", "mixed"]))
    slow_ms = float(test_cfg.get("slow_query_ms", 50))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)

    accounts = ensure_accounts(cfg, 2, runtime_csv, logger, "postgresql")
    accounts = refresh_account_profiles(cfg, accounts)
    owner_uid = int(accounts[0]["uid"])
    peer_uid = int(accounts[1]["uid"])
    group_id = seed_postgresql_data(cfg, owner_uid, peer_uid)

    def worker(i: int):
        workload = str(workload_mix[i % len(workload_mix)])
        started = time.perf_counter()
        stage = "ok"
        ok = True
        conn = None
        try:
            conn = open_postgresql(cfg)
            with conn.cursor() as cursor:
                if workload == "read":
                    cursor.execute(
                        "SELECT msg_id, content, from_uid, to_uid, created_at FROM chat_private_msg "
                        "WHERE conv_uid_min = %s AND conv_uid_max = %s ORDER BY created_at DESC, msg_id DESC LIMIT 20",
                        [min(owner_uid, peer_uid), max(owner_uid, peer_uid)],
                    )
                    cursor.fetchall()
                elif workload == "write":
                    now_ms = int(time.time() * 1000)
                    cursor.execute(
                        "INSERT INTO chat_private_msg(msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, created_at) "
                        "VALUES(%s,%s,%s,%s,%s,%s,%s)",
                        [f"cap-private-{uuid.uuid4().hex}", min(owner_uid, peer_uid), max(owner_uid, peer_uid),
                         owner_uid, peer_uid, f"cap-private-{i}", now_ms],
                    )
                else:
                    now_ms = int(time.time() * 1000)
                    group_seq = time.time_ns()
                    cursor.execute(
                        "SELECT msg_id, content, created_at FROM chat_group_msg WHERE group_id = %s "
                        "ORDER BY group_seq DESC, server_msg_id DESC LIMIT 20",
                        [group_id],
                    )
                    cursor.fetchall()
                    cursor.execute(
                        "INSERT INTO chat_group_msg(msg_id, group_id, from_uid, msg_type, content, created_at, group_seq) "
                        "VALUES(%s,%s,%s,%s,%s,%s,%s)",
                        [f"cap-group-{uuid.uuid4().hex}", group_id, owner_uid, "text", f"cap-group-{i}", now_ms, group_seq],
                    )
                if not conn.autocommit:
                    conn.commit()
        except Exception as exc:  # noqa: BLE001
            ok = False
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "postgresql capacity op failed",
                extra={
                    "event": "loadtest.postgresql_capacity.failed",
                    "scenario": "postgresql_capacity",
                    "stage": stage,
                    "payload": {"workload": workload, "error": str(exc)},
                },
            )
        finally:
            if conn:
                conn.close()
        elapsed_ms = (time.perf_counter() - started) * 1000.0
        return {
            "ok": ok,
            "stage": stage if elapsed_ms <= slow_ms else "slow_query",
            "elapsed_ms": elapsed_ms,
            "phase_ms": {workload: elapsed_ms},
            "mutation": {
                "postgresql_reads": 1 if workload == "read" and ok else 0,
                "postgresql_writes": 1 if workload in ("write", "mixed") and ok else 0,
                "postgresql_slow_queries": 1 if elapsed_ms > slow_ms else 0,
            },
            "sample": {"workload": workload, "elapsed_ms": round(elapsed_ms, 3)},
        }

    result = run_parallel(total, concurrency, worker)
    pg_cfg = get_postgresql_config(cfg)
    report = {
        "scenario": "postgresql_capacity",
        "test": "postgresql_capacity",
        "time_utc": utc_now_str(),
        "target": {"database": pg_cfg.get("database", "memo_pg"), "schema": pg_cfg.get("schema", "public")},
        "config": {
            "total": total,
            "concurrency": concurrency,
            "workload_mix": workload_mix,
            "slow_query_ms": slow_ms,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"seed_group_id": group_id, "seed_owner_uid": owner_uid, "seed_peer_uid": peer_uid},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("postgresql_capacity", report, args.report_path, cfg)
    logger.info(
        "postgresql capacity load test completed",
        extra={
            "event": "loadtest.postgresql_capacity.summary",
            "scenario": "postgresql_capacity",
            "payload": {
                "total": total,
                "concurrency": concurrency,
                "success": result["summary"]["success"],
                "failed": result["summary"]["failed"],
                "rps": result["summary"]["throughput_rps"],
                "p95": result["summary"]["latency_ms"]["p95"],
                "p99": result["summary"]["latency_ms"]["p99"],
                "top_errors": result["top_errors"],
                "report": report_path,
            },
        },
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
