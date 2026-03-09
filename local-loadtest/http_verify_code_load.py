import argparse
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

from memochat_load_common import (
    finalize_report,
    gate_api_url,
    get_log_dir,
    init_json_logger,
    load_json,
    new_trace_id,
    request_verify_code,
    top_errors,
    utc_now_str,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat /get_varifycode load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total requests")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--timeout", type=float, default=0, help="Override timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("http_verify_code_loadtest", log_dir=get_log_dir(cfg))
    auth_cfg = cfg.get("register_verify_reset", {}).get("verify_code", {})
    total = args.total if args.total > 0 else int(auth_cfg.get("total", 300))
    concurrency = args.concurrency if args.concurrency > 0 else int(auth_cfg.get("concurrency", 50))
    timeout = args.timeout if args.timeout > 0 else float(auth_cfg.get("timeout_sec", 5))
    domain = str(cfg.get("account_domain", "loadtest.local"))

    latencies = []
    success = 0
    failed = 0
    error_counter = Counter()

    def one(i: int):
        trace_id = new_trace_id()
        email = f"verify_{utc_now_str()}_{i}_{trace_id[:8]}@{domain}"
        t0 = time.perf_counter()
        ok = False
        stage = "unknown"
        try:
            status, payload = request_verify_code(cfg, email, timeout, trace_id)
            server_error = int(payload.get("error", -1))
            ok = (status == 200 and server_error == 0)
            stage = f"http_{status}_err_{server_error}"
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
        elapsed_ms = (time.perf_counter() - t0) * 1000.0
        return ok, stage, elapsed_ms, trace_id, email

    begin = time.perf_counter()
    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        for ok, stage, elapsed_ms, trace_id, email in executor.map(one, range(total)):
            latencies.append(elapsed_ms)
            if ok:
                success += 1
            else:
                failed += 1
                error_counter[stage] += 1
                logger.warning(
                    "verify code request failed",
                    extra={
                        "event": "loadtest.verify_code.failed",
                        "trace_id": trace_id,
                        "scenario": "auth.verify_code",
                        "stage": stage,
                        "account_email": email,
                    },
                )
    elapsed_sec = time.perf_counter() - begin

    from memochat_load_common import build_summary

    summary = build_summary(latencies, success, failed, elapsed_sec)
    report = {
        "scenario": "auth.verify_code",
        "test": "http_verify_code",
        "time_utc": utc_now_str(),
        "target": gate_api_url(cfg, "/get_varifycode"),
        "config": {
            "total": total,
            "concurrency": concurrency,
            "timeout_sec": timeout,
            "domain": domain,
        },
        "summary": summary,
        "phase_breakdown": {"verify_code": summary["latency_ms"]},
        "error_counter": dict(error_counter),
        "top_errors": top_errors(error_counter),
        "preconditions": {"service": ["GateServer", "VarifyServer", "Redis"]},
        "data_mutation_summary": {"verify_code_requests": success},
    }
    report_path = finalize_report("http_verify_code", report, args.report_path, cfg)

    logger.info(
        "verify code load test completed",
        extra={
            "event": "loadtest.verify_code.summary",
            "scenario": "auth.verify_code",
            "payload": {
                "target": gate_api_url(cfg, "/get_varifycode"),
                "total": total,
                "concurrency": concurrency,
                "success": summary["success"],
                "failed": summary["failed"],
                "success_rate": summary["success_rate"],
                "rps": summary["throughput_rps"],
                "p95": summary["latency_ms"]["p95"],
                "p99": summary["latency_ms"]["p99"],
                "top_errors": dict(error_counter.most_common(8)),
                "report": report_path,
            },
        },
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
