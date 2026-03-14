import argparse
from typing import Dict, List

from memochat_load_common import (
    ensure_accounts,
    finalize_report,
    get_log_dir,
    get_runtime_accounts_csv,
    init_json_logger,
    load_accounts,
    load_json,
    new_trace_id,
    open_mysql,
    register_via_gate,
    request_verify_code,
    save_accounts_csv,
    top_errors,
    upsert_accounts,
    utc_now_str,
    fetch_verify_code_from_redis,
    generate_account,
    run_parallel,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat register load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total requests")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--timeout", type=float, default=0, help="Override timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("http_register_loadtest", log_dir=get_log_dir(cfg))
    register_cfg = cfg.get("register_verify_reset", {}).get("register", {})
    total = args.total if args.total > 0 else int(register_cfg.get("total", 100))
    concurrency = args.concurrency if args.concurrency > 0 else int(register_cfg.get("concurrency", 20))
    timeout = args.timeout if args.timeout > 0 else float(register_cfg.get("timeout_sec", 8))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    seed_prefix = str(register_cfg.get("seed_prefix", "register"))
    domain = str(cfg.get("account_domain", "loadtest.local"))

    generated_accounts: List[Dict[str, str]] = []

    def worker(i: int) -> Dict[str, object]:
        account = generate_account(seed_prefix, domain)
        trace_id = new_trace_id()
        phase_ms = {}
        stage = "unknown"
        ok = False
        started = __import__("time").perf_counter()
        try:
            t0 = __import__("time").perf_counter()
            _, verify_rsp = request_verify_code(cfg, account["email"], timeout, trace_id)
            phase_ms["verify_code"] = (__import__("time").perf_counter() - t0) * 1000.0
            if int(verify_rsp.get("error", -1)) != 0:
                stage = f"verify_err_{verify_rsp.get('error')}"
                return {
                    "ok": False,
                    "stage": stage,
                    "elapsed_ms": (__import__("time").perf_counter() - started) * 1000.0,
                    "phase_ms": phase_ms,
                    "sample": {"email": account["email"]},
                }

            t1 = __import__("time").perf_counter()
            verify_code = fetch_verify_code_from_redis(cfg, account["email"])
            phase_ms["redis_verify_code"] = (__import__("time").perf_counter() - t1) * 1000.0

            t2 = __import__("time").perf_counter()
            _, register_rsp = register_via_gate(
                cfg,
                account["user"],
                account["email"],
                account["password"],
                verify_code,
                timeout,
                trace_id,
            )
            phase_ms["register"] = (__import__("time").perf_counter() - t2) * 1000.0
            if int(register_rsp.get("error", -1)) != 0:
                stage = f"register_err_{register_rsp.get('error')}"
            else:
                account["uid"] = str(register_rsp.get("uid", ""))
                account["user_id"] = str(register_rsp.get("user_id", ""))
                generated_accounts.append(account)
                ok = True
                stage = "ok"
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
        elapsed_ms = (__import__("time").perf_counter() - started) * 1000.0
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": elapsed_ms,
            "phase_ms": phase_ms,
            "mutation": {"accounts_registered": 1 if ok else 0},
            "sample": {"email": account["email"], "uid": account.get("uid", ""), "user_id": account.get("user_id", "")},
        }

    result = run_parallel(total, concurrency, worker)
    existing_accounts = load_accounts(cfg, runtime_csv)
    merged_accounts = upsert_accounts(existing_accounts, generated_accounts)
    if runtime_csv:
        save_accounts_csv(runtime_csv, merged_accounts)

    report = {
        "scenario": "auth.register",
        "test": "http_register",
        "time_utc": utc_now_str(),
        "target": {"verify_code": "/get_varifycode", "register": "/user_register"},
        "config": {
            "total": total,
            "concurrency": concurrency,
            "timeout_sec": timeout,
            "runtime_accounts_csv": runtime_csv,
            "seed_prefix": seed_prefix,
            "domain": domain,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"service": ["GateServer", "VarifyServer", "Redis", "PostgreSQL"]},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("http_register", report, args.report_path, cfg)

    logger.info(
        "register load test completed",
        extra={
            "event": "loadtest.register.summary",
            "scenario": "auth.register",
            "payload": {
                "total": total,
                "concurrency": concurrency,
                "success": result["summary"]["success"],
                "failed": result["summary"]["failed"],
                "success_rate": result["summary"]["success_rate"],
                "rps": result["summary"]["throughput_rps"],
                "p95": result["summary"]["latency_ms"]["p95"],
                "p99": result["summary"]["latency_ms"]["p99"],
                "top_errors": result["top_errors"],
                "report": report_path,
                "runtime_accounts_csv": runtime_csv,
            },
        },
    )

    if merged_accounts:
        conn = open_mysql(cfg)
        conn.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
