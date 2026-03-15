import argparse
from typing import Dict, List

from memochat_load_common import (
    ensure_accounts,
    finalize_report,
    get_log_dir,
    get_runtime_accounts_csv,
    init_json_logger,
    load_json,
    new_trace_id,
    random_password,
    refresh_account_profiles,
    request_verify_code,
    reset_password_via_gate,
    save_accounts_csv,
    top_errors,
    utc_now_str,
    fetch_verify_code_from_redis,
    run_parallel,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat reset password load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total requests")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--timeout", type=float, default=0, help="Override timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("http_reset_password_loadtest", log_dir=get_log_dir(cfg))
    reset_cfg = cfg.get("register_verify_reset", {}).get("reset", {})
    total = args.total if args.total > 0 else int(reset_cfg.get("total", 50))
    concurrency = args.concurrency if args.concurrency > 0 else int(reset_cfg.get("concurrency", 10))
    timeout = args.timeout if args.timeout > 0 else float(reset_cfg.get("timeout_sec", 8))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)

    all_accounts = ensure_accounts(cfg, max(total, concurrency), runtime_csv, logger, "reset")
    all_accounts = refresh_account_profiles(cfg, all_accounts)
    if len(all_accounts) < total:
        total = len(all_accounts)
    accounts = all_accounts[:total]

    def worker(i: int) -> Dict[str, object]:
        account = accounts[i]
        next_password = account.get("last_password", "") or random_password()
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
            _, reset_rsp = reset_password_via_gate(
                cfg,
                account.get("user", account["email"].split("@", 1)[0]),
                account["email"],
                next_password,
                verify_code,
                timeout,
                trace_id,
            )
            phase_ms["reset_password"] = (__import__("time").perf_counter() - t2) * 1000.0
            if int(reset_rsp.get("error", -1)) != 0:
                stage = f"reset_err_{reset_rsp.get('error')}"
            else:
                account["password"] = next_password
                account["last_password"] = random_password()
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
            "mutation": {"passwords_reset": 1 if ok else 0},
            "sample": {"email": account["email"]},
        }

    result = run_parallel(total, concurrency, worker)
    save_accounts_csv(runtime_csv, all_accounts)

    report = {
        "scenario": "auth.reset_password",
        "test": "http_reset_password",
        "time_utc": utc_now_str(),
        "target": {"verify_code": "/get_varifycode", "reset_password": "/reset_pwd"},
        "config": {
            "total": total,
            "concurrency": concurrency,
            "timeout_sec": timeout,
            "runtime_accounts_csv": runtime_csv,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"service": ["GateServer", "VarifyServer", "Redis", "PostgreSQL"], "accounts": len(all_accounts)},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("http_reset_password", report, args.report_path, cfg)

    logger.info(
        "reset password load test completed",
        extra={
            "event": "loadtest.reset_password.summary",
            "scenario": "auth.reset_password",
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
