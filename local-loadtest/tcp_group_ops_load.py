import argparse
import time

from memochat_load_common import (
    ID_APPLY_JOIN_GROUP_REQ,
    ID_APPLY_JOIN_GROUP_RSP,
    ID_CREATE_GROUP_REQ,
    ID_CREATE_GROUP_RSP,
    ID_GROUP_CHAT_MSG_REQ,
    ID_GROUP_CHAT_MSG_RSP,
    ID_INVITE_GROUP_MEMBER_REQ,
    ID_INVITE_GROUP_MEMBER_RSP,
    ID_NOTIFY_GROUP_APPLY_REQ,
    ID_NOTIFY_GROUP_CHAT_MSG_REQ,
    ID_NOTIFY_GROUP_INVITE_REQ,
    ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ,
    ID_REVIEW_GROUP_APPLY_REQ,
    ID_REVIEW_GROUP_APPLY_RSP,
    build_group_create_payload,
    build_group_message_payload,
    connect_chat_client,
    ensure_accounts,
    establish_friendship,
    finalize_report,
    get_log_dir,
    get_runtime_accounts_csv,
    init_json_logger,
    load_json,
    open_mysql,
    refresh_account_profiles,
    run_parallel,
    utc_now_str,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat group operations load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total flows")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--http-timeout", type=float, default=0, help="Override HTTP timeout seconds")
    parser.add_argument("--tcp-timeout", type=float, default=0, help="Override TCP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("tcp_group_ops_loadtest", log_dir=get_log_dir(cfg))
    test_cfg = cfg.get("group_ops", {})
    total = args.total if args.total > 0 else int(test_cfg.get("total", 10))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 2))
    http_timeout = args.http_timeout if args.http_timeout > 0 else float(test_cfg.get("http_timeout_sec", 8))
    tcp_timeout = args.tcp_timeout if args.tcp_timeout > 0 else float(test_cfg.get("tcp_timeout_sec", 8))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)

    accounts = ensure_accounts(cfg, max(total * 3, concurrency * 3), runtime_csv, logger, "group")
    accounts = refresh_account_profiles(cfg, accounts)
    triplets = []
    for idx in range(0, len(accounts) - 2, 3):
        triplets.append((accounts[idx], accounts[idx + 1], accounts[idx + 2]))
        if len(triplets) >= total:
            break
    total = len(triplets)

    def worker(i: int):
        owner, member, applicant = triplets[i]
        started = time.perf_counter()
        owner_client = None
        member_client = None
        applicant_client = None
        phase_ms = {}
        stage = "unknown"
        ok = False
        group_id = 0
        group_code = ""
        trace_id = f"group-{i}-{int(time.time() * 1000)}"
        try:
            owner_client, owner = connect_chat_client(cfg, owner, http_timeout, tcp_timeout, f"{trace_id}-o", f"group-owner-{i}")
            member_client, member = connect_chat_client(cfg, member, http_timeout, tcp_timeout, f"{trace_id}-m", f"group-member-{i}")
            applicant_client, applicant = connect_chat_client(
                cfg, applicant, http_timeout, tcp_timeout, f"{trace_id}-a", f"group-applicant-{i}"
            )

            phase_ms.update(establish_friendship(owner_client, member_client, owner, member, tcp_timeout))

            t0 = time.perf_counter()
            group_name = f"load-group-{i}-{trace_id[-6:]}"
            owner_client.send_json(ID_CREATE_GROUP_REQ, build_group_create_payload(int(owner["uid"]), group_name))
            _, create_rsp = owner_client.recv_until([ID_CREATE_GROUP_RSP], tcp_timeout)
            phase_ms["create_group"] = (time.perf_counter() - t0) * 1000.0
            if int(create_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"create group failed: {create_rsp}")
            group_id = int(create_rsp.get("groupid", 0) or 0)
            group_code = str(create_rsp.get("group_code", ""))

            t1 = time.perf_counter()
            owner_client.send_json(
                ID_INVITE_GROUP_MEMBER_REQ,
                {
                    "fromuid": int(owner["uid"]),
                    "target_user_id": member["user_id"],
                    "groupid": group_id,
                    "reason": "loadtest invite",
                },
            )
            _, invite_rsp = owner_client.recv_until([ID_INVITE_GROUP_MEMBER_RSP], tcp_timeout)
            if int(invite_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"invite member failed: {invite_rsp}")
            member_client.recv_until([ID_NOTIFY_GROUP_INVITE_REQ], tcp_timeout)
            conn = open_mysql(cfg)
            try:
                with conn.cursor() as cursor:
                    cursor.execute(
                        "SELECT apply_id FROM chat_group_apply "
                        "WHERE group_id = %s AND applicant_uid = %s AND type = 1 "
                        "ORDER BY apply_id DESC LIMIT 1",
                        [group_id, int(member["uid"])],
                    )
                    row = cursor.fetchone()
                if not row:
                    raise RuntimeError("invite apply row not found")
                invite_apply_id = int(row["apply_id"])
            finally:
                conn.close()
            owner_client.send_json(
                ID_REVIEW_GROUP_APPLY_REQ,
                {
                    "fromuid": int(owner["uid"]),
                    "apply_id": invite_apply_id,
                    "agree": True,
                },
            )
            _, invite_review_rsp = owner_client.recv_until([ID_REVIEW_GROUP_APPLY_RSP], tcp_timeout)
            if int(invite_review_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"review invite failed: {invite_review_rsp}")
            member_client.recv_until([ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ], tcp_timeout)
            phase_ms["invite_member"] = (time.perf_counter() - t1) * 1000.0

            t2 = time.perf_counter()
            applicant_client.send_json(
                ID_APPLY_JOIN_GROUP_REQ,
                {
                    "fromuid": int(applicant["uid"]),
                    "group_code": group_code,
                    "reason": "loadtest apply",
                },
            )
            _, apply_rsp = applicant_client.recv_until([ID_APPLY_JOIN_GROUP_RSP], tcp_timeout)
            if int(apply_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"apply join group failed: {apply_rsp}")
            owner_client.recv_until([ID_NOTIFY_GROUP_APPLY_REQ], tcp_timeout)
            phase_ms["apply_join_group"] = (time.perf_counter() - t2) * 1000.0

            t3 = time.perf_counter()
            conn = open_mysql(cfg)
            try:
                with conn.cursor() as cursor:
                    cursor.execute(
                        "SELECT apply_id FROM chat_group_apply WHERE group_id = %s AND applicant_uid = %s ORDER BY apply_id DESC LIMIT 1",
                        [group_id, int(applicant["uid"])],
                    )
                    row = cursor.fetchone()
                if not row:
                    raise RuntimeError("group apply row not found")
                apply_id = int(row["apply_id"])
            finally:
                conn.close()
            owner_client.send_json(
                ID_REVIEW_GROUP_APPLY_REQ,
                {
                    "fromuid": int(owner["uid"]),
                    "apply_id": apply_id,
                    "agree": True,
                },
            )
            _, review_rsp = owner_client.recv_until([ID_REVIEW_GROUP_APPLY_RSP], tcp_timeout)
            if int(review_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"review group apply failed: {review_rsp}")
            applicant_client.recv_until([ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ], tcp_timeout)
            phase_ms["review_group_apply"] = (time.perf_counter() - t3) * 1000.0

            t4 = time.perf_counter()
            owner_client.send_json(
                ID_GROUP_CHAT_MSG_REQ,
                build_group_message_payload(int(owner["uid"]), group_id, f"group-message-{trace_id}", "text"),
            )
            _, group_rsp = owner_client.recv_until([ID_GROUP_CHAT_MSG_RSP], tcp_timeout)
            if int(group_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"group message send failed: {group_rsp}")
            member_client.recv_until([ID_NOTIFY_GROUP_CHAT_MSG_REQ], tcp_timeout)
            applicant_client.recv_until([ID_NOTIFY_GROUP_CHAT_MSG_REQ], tcp_timeout)
            phase_ms["group_message"] = (time.perf_counter() - t4) * 1000.0

            conn = open_mysql(cfg)
            try:
                with conn.cursor() as cursor:
                    cursor.execute(
                        "SELECT COUNT(1) AS cnt FROM chat_group_member WHERE group_id = %s AND status = 1",
                        [group_id],
                    )
                    member_row = cursor.fetchone()
                    cursor.execute(
                        "SELECT COUNT(1) AS cnt FROM chat_group_msg WHERE group_id = %s",
                        [group_id],
                    )
                    msg_row = cursor.fetchone()
                ok = int(member_row["cnt"]) >= 3 and int(msg_row["cnt"]) >= 1
            finally:
                conn.close()
            stage = "ok" if ok else "db_verify_failed"
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "group ops flow failed",
                extra={
                    "event": "loadtest.group_ops.failed",
                    "scenario": "group_ops",
                    "stage": stage,
                    "trace_id": trace_id,
                    "account_email": owner.get("email", ""),
                    "group_id": str(group_id),
                    "payload": {"error": str(exc), "group_code": group_code},
                },
            )
        finally:
            for client in (owner_client, member_client, applicant_client):
                if client:
                    client.close()
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": (time.perf_counter() - started) * 1000.0,
            "phase_ms": phase_ms,
            "mutation": {"groups_created": 1 if group_id > 0 else 0, "group_messages_sent": 1 if ok else 0},
            "sample": {"group_id": group_id, "group_code": group_code, "owner_email": owner.get("email", "")},
        }

    result = run_parallel(total, min(concurrency, max(1, total)), worker)
    report = {
        "scenario": "group_ops",
        "test": "tcp_group_ops",
        "time_utc": utc_now_str(),
        "target": {"tcp": [1031, 1035, 1038, 1041, 1044]},
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
        "preconditions": {"accounts": total * 3, "service": ["GateServer", "StatusServer", "ConfiguredChatNodes"]},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("tcp_group_ops", report, args.report_path, cfg)

    logger.info(
        "group ops load test completed",
        extra={
            "event": "loadtest.group_ops.summary",
            "scenario": "group_ops",
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
