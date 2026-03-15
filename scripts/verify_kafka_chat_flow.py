import argparse
import json
import sys
from typing import Dict, List


REPO_ROOT = r"d:\MemoChat-Qml-Drogon"
sys.path.insert(0, rf"{REPO_ROOT}\local-loadtest")

from memochat_load_common import (  # noqa: E402
    ID_CREATE_GROUP_REQ,
    ID_CREATE_GROUP_RSP,
    ID_GROUP_CHAT_MSG_REQ,
    ID_GROUP_CHAT_MSG_RSP,
    ID_GROUP_HISTORY_REQ,
    ID_GROUP_HISTORY_RSP,
    ID_NOTIFY_GROUP_CHAT_MSG_REQ,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ,
    ID_PRIVATE_HISTORY_REQ,
    ID_PRIVATE_HISTORY_RSP,
    ID_TEXT_CHAT_MSG_REQ,
    ID_TEXT_CHAT_MSG_RSP,
    build_group_create_payload,
    build_group_message_payload,
    build_private_text_payload,
    connect_chat_client,
    expect_response_status,
    get_runtime_accounts_csv,
    load_accounts,
    load_json,
    refresh_account_profiles,
    wait_for_message_status,
)


def compact(value: Dict) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def select_accounts(cfg: Dict, accounts_csv: str) -> List[Dict[str, str]]:
    accounts = load_accounts(cfg, accounts_csv)
    accounts = refresh_account_profiles(cfg, accounts) if accounts else []
    valid = [one for one in accounts if one.get("email") and one.get("password") and one.get("uid") and one.get("user_id")]
    if len(valid) < 2:
        raise RuntimeError(f"need at least 2 valid accounts in {accounts_csv}")
    return valid[:2]


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify Kafka-backed MemoChat private/group message flow")
    parser.add_argument("--config", default=rf"{REPO_ROOT}\local-loadtest\config.json", help="Path to loadtest config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--timeout", type=float, default=8.0, help="HTTP/TCP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    account_a, account_b = select_accounts(cfg, runtime_csv)

    client_a = None
    client_b = None
    try:
        client_a, info_a = connect_chat_client(cfg, account_a, args.timeout, args.timeout, "verify-kafka-a", "verify-kafka-a")
        client_b, info_b = connect_chat_client(cfg, account_b, args.timeout, args.timeout, "verify-kafka-b", "verify-kafka-b")

        print("connected", info_a["uid"], info_b["uid"])

        private_payload = build_private_text_payload(
            int(info_a["uid"]),
            int(info_b["uid"]),
            "verify-kafka-private",
        )
        private_msg_id = str(private_payload["text_array"][0]["msgid"])
        client_a.send_json(ID_TEXT_CHAT_MSG_REQ, private_payload)
        _, private_rsp = client_a.recv_until([ID_TEXT_CHAT_MSG_RSP], args.timeout)
        private_rsp_status = expect_response_status(private_rsp)
        private_status = wait_for_message_status(client_a, private_msg_id, "private", args.timeout)
        _, private_notify = client_b.recv_until([ID_NOTIFY_TEXT_CHAT_MSG_REQ], args.timeout)

        client_a.send_json(
            ID_PRIVATE_HISTORY_REQ,
            {"fromuid": int(info_a["uid"]), "peer_uid": int(info_b["uid"]), "before_ts": 0, "limit": 20},
        )
        _, private_history_rsp = client_a.recv_until([ID_PRIVATE_HISTORY_RSP], args.timeout)
        private_history_messages = list(private_history_rsp.get("messages") or [])
        if not any(str(one.get("msgid", "")) == private_msg_id for one in private_history_messages):
            raise RuntimeError("private history does not contain kafka probe message")

        client_a.send_json(
            ID_CREATE_GROUP_REQ,
            build_group_create_payload(int(info_a["uid"]), "verify-kafka-group", [info_b["user_id"]]),
        )
        _, create_rsp = client_a.recv_until([ID_CREATE_GROUP_RSP], args.timeout)
        group_id = int(create_rsp.get("groupid", 0) or 0)
        if group_id <= 0:
            raise RuntimeError(f"group create failed: {create_rsp}")
        try:
            client_b.recv_until([1037, 1043], 3.0)
        except Exception:
            pass

        group_payload = build_group_message_payload(
            int(info_a["uid"]),
            group_id,
            "verify-kafka-group-message",
        )
        group_msg_id = str(group_payload["msg"]["msgid"])
        client_a.send_json(ID_GROUP_CHAT_MSG_REQ, group_payload)
        _, group_rsp = client_a.recv_until([ID_GROUP_CHAT_MSG_RSP], args.timeout)
        group_rsp_status = expect_response_status(group_rsp)
        group_status = wait_for_message_status(client_a, group_msg_id, "group", args.timeout)
        _, group_notify = client_b.recv_until([ID_NOTIFY_GROUP_CHAT_MSG_REQ], args.timeout)

        client_a.send_json(
            ID_GROUP_HISTORY_REQ,
            {"fromuid": int(info_a["uid"]), "groupid": group_id, "before_ts": 0, "before_seq": 0, "limit": 20},
        )
        _, group_history_rsp = client_a.recv_until([ID_GROUP_HISTORY_RSP], args.timeout)
        group_history_messages = list(group_history_rsp.get("messages") or [])
        if not any(str(one.get("msgid", "")) == group_msg_id for one in group_history_messages):
            raise RuntimeError("group history does not contain kafka probe message")

        result = {
            "ok": True,
            "accounts": {
                "sender_email": info_a["email"],
                "peer_email": info_b["email"],
                "sender_uid": int(info_a["uid"]),
                "peer_uid": int(info_b["uid"]),
            },
            "private": {
                "response_status": private_rsp_status,
                "response": private_rsp,
                "persisted": private_status,
                "notify": private_notify,
                "history_count": len(private_history_messages),
            },
            "group": {
                "group_id": group_id,
                "response_status": group_rsp_status,
                "response": group_rsp,
                "persisted": group_status,
                "notify": group_notify,
                "history_count": len(group_history_messages),
            },
        }
        print(compact(result))
        return 0
    finally:
        if client_a:
            client_a.close()
        if client_b:
            client_b.close()


if __name__ == "__main__":
    raise SystemExit(main())
