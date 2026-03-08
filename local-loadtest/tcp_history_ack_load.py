import argparse
import time

from memochat_load_common import (
    ID_CREATE_GROUP_REQ,
    ID_CREATE_GROUP_RSP,
    ID_GROUP_CHAT_MSG_REQ,
    ID_GROUP_CHAT_MSG_RSP,
    ID_GROUP_HISTORY_REQ,
    ID_GROUP_HISTORY_RSP,
    ID_GROUP_READ_ACK_REQ,
    ID_NOTIFY_GROUP_CHAT_MSG_REQ,
    ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ,
    ID_NOTIFY_PRIVATE_READ_ACK_REQ,
    ID_PRIVATE_HISTORY_REQ,
    ID_PRIVATE_HISTORY_RSP,
    ID_PRIVATE_READ_ACK_REQ,
    ID_TEXT_CHAT_MSG_REQ,
    ID_TEXT_CHAT_MSG_RSP,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ,
    build_group_create_payload,
    build_group_message_payload,
    build_private_text_payload,
    connect_chat_client,
    ensure_accounts,
    establish_friendship,
    finalize_report,
    get_runtime_accounts_csv,
    init_json_logger,
    load_json,
    open_mysql,
    refresh_account_profiles,
    run_parallel,
    utc_now_str,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat history and read-ack load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total flows")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--http-timeout", type=float, default=0, help="Override HTTP timeout seconds")
    parser.add_argument("--tcp-timeout", type=float, default=0, help="Override TCP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("tcp_history_ack_loadtest", log_dir="logs")
    test_cfg = cfg.get("history_ack", {})
    total = args.total if args.total > 0 else int(test_cfg.get("total", 8))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 2))
    http_timeout = args.http_timeout if args.http_timeout > 0 else float(test_cfg.get("http_timeout_sec", 8))
    tcp_timeout = args.tcp_timeout if args.tcp_timeout > 0 else float(test_cfg.get("tcp_timeout_sec", 8))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)

    accounts = ensure_accounts(cfg, max(total * 2, concurrency * 2), runtime_csv, logger, "history")
    accounts = refresh_account_profiles(cfg, accounts)
    pairs = []
    for idx in range(0, len(accounts) - 1, 2):
        pairs.append((accounts[idx], accounts[idx + 1]))
        if len(pairs) >= total:
            break
    total = len(pairs)

    def worker(i: int):
        owner, peer = pairs[i]
        owner_client = None
        peer_client = None
        phase_ms = {}
        group_id = 0
        trace_id = f"history-{i}-{int(time.time() * 1000)}"
        ok = False
        stage = "unknown"
        started = time.perf_counter()
        try:
            owner_client, owner = connect_chat_client(cfg, owner, http_timeout, tcp_timeout, f"{trace_id}-o", f"history-owner-{i}")
            peer_client, peer = connect_chat_client(cfg, peer, http_timeout, tcp_timeout, f"{trace_id}-p", f"history-peer-{i}")

            phase_ms.update(establish_friendship(owner_client, peer_client, owner, peer, tcp_timeout))

            t0 = time.perf_counter()
            for msg_index in range(2):
                owner_client.send_json(
                    ID_TEXT_CHAT_MSG_REQ,
                    build_private_text_payload(int(owner["uid"]), int(peer["uid"]), f"owner-to-peer-{i}-{msg_index}"),
                )
                owner_client.recv_until([ID_TEXT_CHAT_MSG_RSP], tcp_timeout)
                peer_client.recv_until([ID_NOTIFY_TEXT_CHAT_MSG_REQ], tcp_timeout)
                peer_client.send_json(
                    ID_TEXT_CHAT_MSG_REQ,
                    build_private_text_payload(int(peer["uid"]), int(owner["uid"]), f"peer-to-owner-{i}-{msg_index}"),
                )
                peer_client.recv_until([ID_TEXT_CHAT_MSG_RSP], tcp_timeout)
                owner_client.recv_until([ID_NOTIFY_TEXT_CHAT_MSG_REQ], tcp_timeout)
            phase_ms["private_message_warmup"] = (time.perf_counter() - t0) * 1000.0

            t1 = time.perf_counter()
            owner_client.send_json(
                ID_CREATE_GROUP_REQ,
                build_group_create_payload(int(owner["uid"]), f"history-group-{i}", [peer["user_id"]]),
            )
            _, create_rsp = owner_client.recv_until([ID_CREATE_GROUP_RSP], tcp_timeout)
            if int(create_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"create group failed: {create_rsp}")
            group_id = int(create_rsp.get("groupid", 0) or 0)
            peer_client.recv_until([1037], tcp_timeout)
            phase_ms["group_create"] = (time.perf_counter() - t1) * 1000.0

            t2 = time.perf_counter()
            owner_client.send_json(
                ID_GROUP_CHAT_MSG_REQ,
                build_group_message_payload(int(owner["uid"]), group_id, f"group-owner-{trace_id}"),
            )
            owner_client.recv_until([ID_GROUP_CHAT_MSG_RSP], tcp_timeout)
            peer_client.recv_until([ID_NOTIFY_GROUP_CHAT_MSG_REQ], tcp_timeout)
            peer_client.send_json(
                ID_GROUP_CHAT_MSG_REQ,
                build_group_message_payload(int(peer["uid"]), group_id, f"group-peer-{trace_id}"),
            )
            peer_client.recv_until([ID_GROUP_CHAT_MSG_RSP], tcp_timeout)
            owner_client.recv_until([ID_NOTIFY_GROUP_CHAT_MSG_REQ], tcp_timeout)
            phase_ms["group_message_warmup"] = (time.perf_counter() - t2) * 1000.0

            t3 = time.perf_counter()
            owner_client.send_json(
                ID_PRIVATE_HISTORY_REQ,
                {"fromuid": int(owner["uid"]), "peer_uid": int(peer["uid"]), "before_ts": 0, "limit": 20},
            )
            _, private_history_rsp = owner_client.recv_until([ID_PRIVATE_HISTORY_RSP], tcp_timeout)
            if int(private_history_rsp.get("error", -1)) != 0 or not private_history_rsp.get("messages"):
                raise RuntimeError(f"private history failed: {private_history_rsp}")
            phase_ms["private_history"] = (time.perf_counter() - t3) * 1000.0

            t4 = time.perf_counter()
            read_ts = int(time.time() * 1000)
            owner_client.send_json(
                ID_PRIVATE_READ_ACK_REQ,
                {"fromuid": int(owner["uid"]), "peer_uid": int(peer["uid"]), "read_ts": read_ts},
            )
            _, private_ack_notify = peer_client.recv_until([ID_NOTIFY_PRIVATE_READ_ACK_REQ], tcp_timeout)
            if int(private_ack_notify.get("error", -1)) != 0:
                raise RuntimeError(f"private read ack notify failed: {private_ack_notify}")
            phase_ms["private_read_ack"] = (time.perf_counter() - t4) * 1000.0

            t5 = time.perf_counter()
            owner_client.send_json(
                ID_GROUP_HISTORY_REQ,
                {"fromuid": int(owner["uid"]), "groupid": group_id, "before_ts": 0, "before_seq": 0, "limit": 20},
            )
            _, group_history_rsp = owner_client.recv_until([ID_GROUP_HISTORY_RSP], tcp_timeout)
            if int(group_history_rsp.get("error", -1)) != 0 or not group_history_rsp.get("messages"):
                raise RuntimeError(f"group history failed: {group_history_rsp}")
            phase_ms["group_history"] = (time.perf_counter() - t5) * 1000.0

            t6 = time.perf_counter()
            owner_client.send_json(
                ID_GROUP_READ_ACK_REQ,
                {"fromuid": int(owner["uid"]), "groupid": group_id, "read_ts": int(time.time() * 1000)},
            )
            _, group_ack_notify = peer_client.recv_until([ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ], tcp_timeout)
            if group_ack_notify.get("event") != "group_read_ack":
                raise RuntimeError(f"group read ack notify mismatch: {group_ack_notify}")
            phase_ms["group_read_ack"] = (time.perf_counter() - t6) * 1000.0

            conn = open_mysql(cfg)
            try:
                with conn.cursor() as cursor:
                    cursor.execute(
                        "SELECT COUNT(1) AS cnt FROM chat_private_read_state WHERE uid = %s AND peer_uid = %s",
                        [int(owner["uid"]), int(peer["uid"])],
                    )
                    private_row = cursor.fetchone()
                    cursor.execute(
                        "SELECT COUNT(1) AS cnt FROM chat_group_read_state WHERE uid = %s AND group_id = %s",
                        [int(owner["uid"]), group_id],
                    )
                    group_row = cursor.fetchone()
                ok = int(private_row["cnt"]) >= 1 and int(group_row["cnt"]) >= 1
            finally:
                conn.close()
            stage = "ok" if ok else "db_verify_failed"
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "history/ack flow failed",
                extra={
                    "event": "loadtest.history_ack.failed",
                    "scenario": "history_ack",
                    "stage": stage,
                    "trace_id": trace_id,
                    "account_email": owner.get("email", ""),
                    "group_id": str(group_id),
                    "payload": {"error": str(exc)},
                },
            )
        finally:
            for client in (owner_client, peer_client):
                if client:
                    client.close()
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": (time.perf_counter() - started) * 1000.0,
            "phase_ms": phase_ms,
            "mutation": {"history_flows_completed": 1 if ok else 0},
            "sample": {"owner_email": owner.get("email", ""), "peer_email": peer.get("email", ""), "group_id": group_id},
        }

    result = run_parallel(total, min(concurrency, max(1, total)), worker)
    report = {
        "scenario": "history_ack",
        "test": "tcp_history_ack",
        "time_utc": utc_now_str(),
        "target": {"tcp": [1017, 1044, 1059, 1047, 1076, 1071]},
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
    report_path = finalize_report("tcp_history_ack", report, args.report_path)

    logger.info(
        "history/ack load test completed",
        extra={
            "event": "loadtest.history_ack.summary",
            "scenario": "history_ack",
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
