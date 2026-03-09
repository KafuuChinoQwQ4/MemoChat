import argparse
import time

from memochat_load_common import finalize_report, get_log_dir, init_json_logger, load_json, open_redis, run_parallel, utc_now_str


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat Redis capacity load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total operations")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("redis_capacity_loadtest", log_dir=get_log_dir(cfg))
    test_cfg = cfg.get("redis_capacity", {})
    total = args.total if args.total > 0 else int(test_cfg.get("total", 1000))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 50))
    workload_mix = list(test_cfg.get("workload_mix", ["verify_code", "token_session", "online_route"]))
    pipeline_mode = bool(test_cfg.get("pipeline_mode", True))

    def worker(i: int):
        workload = str(workload_mix[i % len(workload_mix)])
        key_prefix = {
            "verify_code": "code_redisload_",
            "token_session": "utoken_redisload_",
            "online_route": "uip_redisload_",
        }.get(workload, "load_")
        key = f"{key_prefix}{i}"
        value = f"value-{i}"
        redis_conn = None
        ok = True
        stage = "ok"
        started = time.perf_counter()
        try:
            redis_conn = open_redis(cfg)
            if pipeline_mode:
                pipe = redis_conn.pipeline()
                pipe.set(key, value, ex=600)
                pipe.get(key)
                pipe.expire(key, 600)
                pipe.delete(key)
                pipe.execute()
            else:
                redis_conn.set(key, value, ex=600)
                redis_conn.get(key)
                redis_conn.expire(key, 600)
                redis_conn.delete(key)
        except Exception as exc:  # noqa: BLE001
            ok = False
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "redis capacity op failed",
                extra={
                    "event": "loadtest.redis_capacity.failed",
                    "scenario": "redis_capacity",
                    "stage": stage,
                    "payload": {"workload": workload, "error": str(exc)},
                },
            )
        finally:
            if redis_conn:
                try:
                    redis_conn.close()
                except Exception:
                    pass
        elapsed_ms = (time.perf_counter() - started) * 1000.0
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": elapsed_ms,
            "phase_ms": {workload: elapsed_ms},
            "mutation": {f"redis_{workload}_ops": 1 if ok else 0},
            "sample": {"workload": workload, "elapsed_ms": round(elapsed_ms, 3)},
        }

    result = run_parallel(total, concurrency, worker)
    report = {
        "scenario": "redis_capacity",
        "test": "redis_capacity",
        "time_utc": utc_now_str(),
        "target": {"redis": cfg.get("redis", {}).get("host", "127.0.0.1")},
        "config": {
            "total": total,
            "concurrency": concurrency,
            "workload_mix": workload_mix,
            "pipeline_mode": pipeline_mode,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"workloads": workload_mix},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("redis_capacity", report, args.report_path, cfg)
    logger.info(
        "redis capacity load test completed",
        extra={
            "event": "loadtest.redis_capacity.summary",
            "scenario": "redis_capacity",
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
