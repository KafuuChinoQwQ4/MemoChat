import argparse
import time

from memochat_load_common import (
    ID_NOTIFY_TEXT_CHAT_MSG_REQ,
    ID_TEXT_CHAT_MSG_REQ,
    ID_TEXT_CHAT_MSG_RSP,
    build_private_text_payload,
    connect_chat_client,
    default_join_url,
    encode_call_invite,
    ensure_accounts,
    establish_friendship,
    finalize_report,
    get_runtime_accounts_csv,
    init_json_logger,
    load_json,
    refresh_account_profiles,
    run_parallel,
    utc_now_str,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat call invite load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total flows")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--http-timeout", type=float, default=0, help="Override HTTP timeout seconds")
    parser.add_argument("--tcp-timeout", type=float, default=0, help="Override TCP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("tcp_call_invite_loadtest", log_dir="logs")
    test_cfg = cfg.get("call_invite", {})
    total = args.total if args.total > 0 else int(test_cfg.get("total", 10))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 2))
    http_timeout = args.http_timeout if args.http_timeout > 0 else float(test_cfg.get("http_timeout_sec", 8))
    tcp_timeout = args.tcp_timeout if args.tcp_timeout > 0 else float(test_cfg.get("tcp_timeout_sec", 8))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    call_types = list(test_cfg.get("call_types", ["voice", "video"]))

    accounts = ensure_accounts(cfg, max(total * 2, concurrency * 2), runtime_csv, logger, "call")
    accounts = refresh_account_profiles(cfg, accounts)
    pairs = []
    for idx in range(0, len(accounts) - 1, 2):
        pairs.append((accounts[idx], accounts[idx + 1]))
        if len(pairs) >= total:
            break
    total = len(pairs)

    def worker(i: int):
        caller, callee = pairs[i]
        caller_client = None
        callee_client = None
        phase_ms = {}
        ok = False
        stage = "unknown"
        call_type = str(call_types[i % len(call_types)])
        trace_id = f"call-{i}-{int(time.time() * 1000)}"
        started = time.perf_counter()
        try:
            caller_client, caller = connect_chat_client(cfg, caller, http_timeout, tcp_timeout, f"{trace_id}-c", f"call-caller-{i}")
            callee_client, callee = connect_chat_client(cfg, callee, http_timeout, tcp_timeout, f"{trace_id}-r", f"call-callee-{i}")
            phase_ms.update(establish_friendship(caller_client, callee_client, caller, callee, tcp_timeout))

            join_url = default_join_url("video" if call_type == "video" else "voice", int(caller["uid"]), int(callee["uid"]))
            content = encode_call_invite("video" if call_type == "video" else "voice", join_url)

            t0 = time.perf_counter()
            caller_client.send_json(
                ID_TEXT_CHAT_MSG_REQ,
                build_private_text_payload(int(caller["uid"]), int(callee["uid"]), content),
            )
            _, send_rsp = caller_client.recv_until([ID_TEXT_CHAT_MSG_RSP], tcp_timeout)
            if int(send_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"send call invite failed: {send_rsp}")
            _, notify = callee_client.recv_until([ID_NOTIFY_TEXT_CHAT_MSG_REQ], tcp_timeout)
            text_array = notify.get("text_array") or []
            first = text_array[0] if text_array else {}
            ok = bool(text_array) and str(first.get("content", "")).startswith("__memochat_call__:")
            stage = "ok" if ok else "content_mismatch"
            phase_ms["call_invite"] = (time.perf_counter() - t0) * 1000.0
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "call invite flow failed",
                extra={
                    "event": "loadtest.call_invite.failed",
                    "scenario": "call_invite",
                    "stage": stage,
                    "trace_id": trace_id,
                    "account_email": caller.get("email", ""),
                    "peer_uid": callee.get("uid", ""),
                    "payload": {"error": str(exc), "call_type": call_type},
                },
            )
        finally:
            if caller_client:
                caller_client.close()
            if callee_client:
                callee_client.close()
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": (time.perf_counter() - started) * 1000.0,
            "phase_ms": phase_ms,
            "mutation": {"call_invites_sent": 1 if ok else 0},
            "sample": {"caller_email": caller.get("email", ""), "callee_email": callee.get("email", ""), "call_type": call_type},
        }

    result = run_parallel(total, min(concurrency, max(1, total)), worker)
    report = {
        "scenario": "call_invite",
        "test": "tcp_call_invite",
        "time_utc": utc_now_str(),
        "target": {"tcp": [1017, 1019]},
        "config": {
            "total": total,
            "concurrency": min(concurrency, max(1, total)),
            "http_timeout_sec": http_timeout,
            "tcp_timeout_sec": tcp_timeout,
            "runtime_accounts_csv": runtime_csv,
            "call_types": call_types,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"accounts": total * 2, "service": ["GateServer", "StatusServer", "ConfiguredChatNodes"]},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("tcp_call_invite", report, args.report_path)

    logger.info(
        "call invite load test completed",
        extra={
            "event": "loadtest.call_invite.summary",
            "scenario": "call_invite",
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
