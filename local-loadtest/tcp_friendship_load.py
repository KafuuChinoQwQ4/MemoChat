import argparse
import time

from memochat_load_common import (
    connect_chat_client,
    db_verify_friendship,
    ensure_accounts,
    establish_friendship,
    finalize_report,
    get_runtime_accounts_csv,
    init_json_logger,
    load_json,
    open_mysql,
    refresh_account_profiles,
    run_parallel,
    select_non_friend_pairs,
    utc_now_str,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat friendship load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total flows")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--http-timeout", type=float, default=0, help="Override HTTP timeout seconds")
    parser.add_argument("--tcp-timeout", type=float, default=0, help="Override TCP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("tcp_friendship_loadtest", log_dir="logs")
    test_cfg = cfg.get("friendship", {})
    total = args.total if args.total > 0 else int(test_cfg.get("total", 20))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 5))
    http_timeout = args.http_timeout if args.http_timeout > 0 else float(test_cfg.get("http_timeout_sec", 8))
    tcp_timeout = args.tcp_timeout if args.tcp_timeout > 0 else float(test_cfg.get("tcp_timeout_sec", 8))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)

    accounts = ensure_accounts(cfg, max(total * 2, concurrency * 2), runtime_csv, logger, "friend")
    accounts = refresh_account_profiles(cfg, accounts)

    conn = open_mysql(cfg)
    try:
        pairs = select_non_friend_pairs(conn, accounts, total)
    finally:
        conn.close()
    total = len(pairs)

    def worker(i: int):
        requester, approver = pairs[i]
        trace_id = f"friend-{i}-{int(time.time() * 1000)}"
        started = time.perf_counter()
        requester_client = None
        approver_client = None
        phase_ms = {}
        stage = "unknown"
        ok = False
        try:
            requester_client, requester = connect_chat_client(
                cfg, requester, http_timeout, tcp_timeout, f"{trace_id}-a", f"friend-a-{i}"
            )
            approver_client, approver = connect_chat_client(
                cfg, approver, http_timeout, tcp_timeout, f"{trace_id}-b", f"friend-b-{i}"
            )
            phase_ms.update(establish_friendship(requester_client, approver_client, requester, approver, tcp_timeout))

            conn = open_mysql(cfg)
            try:
                ok = db_verify_friendship(conn, int(requester["uid"]), int(approver["uid"]))
            finally:
                conn.close()
            stage = "ok" if ok else "db_verify_failed"
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "friendship flow failed",
                extra={
                    "event": "loadtest.friendship.failed",
                    "scenario": "friendship",
                    "stage": stage,
                    "trace_id": trace_id,
                    "account_email": requester.get("email", ""),
                    "payload": {
                        "requester_email": requester.get("email", ""),
                        "approver_email": approver.get("email", ""),
                        "error": str(exc),
                    },
                },
            )
        finally:
            if requester_client:
                requester_client.close()
            if approver_client:
                approver_client.close()
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": (time.perf_counter() - started) * 1000.0,
            "phase_ms": phase_ms,
            "mutation": {"friend_pairs_established": 1 if ok else 0},
            "sample": {
                "requester_email": requester.get("email", ""),
                "approver_email": approver.get("email", ""),
            },
        }

    result = run_parallel(total, min(concurrency, max(1, total)), worker)
    report = {
        "scenario": "friendship",
        "test": "tcp_friendship",
        "time_utc": utc_now_str(),
        "target": {"gate_login": "/user_login", "tcp": [1007, 1009, 1013]},
        "config": {
            "total": total,
            "concurrency": min(concurrency, max(1, total)),
            "http_timeout_sec": http_timeout,
            "tcp_timeout_sec": tcp_timeout,
            "runtime_accounts_csv": runtime_csv,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"accounts": total * 2, "service": ["GateServer", "StatusServer", "ChatServer", "ChatServer2"]},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("tcp_friendship", report, args.report_path)

    logger.info(
        "friendship load test completed",
        extra={
            "event": "loadtest.friendship.summary",
            "scenario": "friendship",
            "payload": {
                "total": total,
                "concurrency": min(concurrency, max(1, total)),
                "success": result["summary"]["success"],
                "failed": result["summary"]["failed"],
                "success_rate": result["summary"]["success_rate"],
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
