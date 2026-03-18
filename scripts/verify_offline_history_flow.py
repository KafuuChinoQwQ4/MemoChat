import argparse
import json
import sys
from typing import Dict, List, Tuple


REPO_ROOT = r"d:\MemoChat-Qml-Drogon"
sys.path.insert(0, rf"{REPO_ROOT}\local-loadtest")

from memochat_load_common import (  # noqa: E402
    ID_CREATE_GROUP_REQ,
    ID_CREATE_GROUP_RSP,
    ID_GROUP_CHAT_MSG_REQ,
    ID_GROUP_CHAT_MSG_RSP,
    ID_GROUP_HISTORY_REQ,
    ID_GROUP_HISTORY_RSP,
    ID_PRIVATE_HISTORY_REQ,
    ID_PRIVATE_HISTORY_RSP,
    ID_TEXT_CHAT_MSG_REQ,
    ID_TEXT_CHAT_MSG_RSP,
    build_group_create_payload,
    build_group_message_payload,
    build_private_text_payload,
    connect_chat_client,
    establish_friendship,
    expect_response_status,
    get_runtime_accounts_csv,
    load_accounts,
    load_json,
    refresh_account_profiles,
    request_verify_code,
    fetch_verify_code_from_redis,
    register_via_gate,
    now_ms,
)


def compact(value: Dict) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def select_accounts(cfg: Dict, accounts_csv: str) -> Tuple[Dict[str, str], Dict[str, str]]:
    accounts = load_accounts(cfg, accounts_csv)
    accounts = refresh_account_profiles(cfg, accounts) if accounts else []
    valid = [one for one in accounts if one.get("email") and one.get("password") and one.get("uid") and one.get("user_id")]
    if len(valid) < 2:
        raise RuntimeError(f"need at least 2 valid accounts in {accounts_csv}")
    return valid[0], valid[1]


def ensure_working_accounts(cfg: Dict, accounts_csv: str, timeout: float) -> Tuple[Dict[str, str], Dict[str, str]]:
    """
    Try runtime CSV first; if login keeps failing (e.g. rotated passwords out of sync),
    register two fresh accounts via Gate and return them.
    """
    account_a, account_b = select_accounts(cfg, accounts_csv)
    try:
        pre_client, _ = connect_with_password_fallback(cfg, account_a, timeout, timeout, "verify-offline-precheck", "verify-offline-precheck")
        pre_client.close()
        return account_a, account_b
    except Exception:
        pass

    ts = now_ms()
    domain = str(cfg.get("account_domain", "loadtest.local")).strip() or "loadtest.local"
    password = f"Pwd{ts % 1000000}!"
    user_a = f"offline_a_{ts}"
    user_b = f"offline_b_{ts}"
    email_a = f"{user_a}@{domain}"
    email_b = f"{user_b}@{domain}"

    for user, email in [(user_a, email_a), (user_b, email_b)]:
        status, rsp = request_verify_code(cfg, email, timeout, f"verify-offline-code-{user}")
        if status != 200 or int(rsp.get("error", -1)) != 0:
            raise RuntimeError(f"request verify code failed for {email}: status={status}, rsp={rsp}")
        code = fetch_verify_code_from_redis(cfg, email, 5.0)
        status, reg_rsp = register_via_gate(cfg, user, email, password, code, timeout, f"verify-offline-reg-{user}")
        if status != 200 or int(reg_rsp.get("error", -1)) != 0:
            raise RuntimeError(f"register failed for {email}: status={status}, rsp={reg_rsp}")

    return (
        {"email": email_a, "password": password, "user": user_a},
        {"email": email_b, "password": password, "user": user_b},
    )


def connect_with_password_fallback(
    cfg: Dict,
    account: Dict[str, str],
    http_timeout: float,
    tcp_timeout: float,
    trace_id: str,
    label: str,
):
    attempts: List[Dict[str, str]] = []
    base = dict(account)
    attempts.append(base)
    last_pwd = str(account.get("last_password", "") or "").strip()
    if last_pwd and last_pwd != str(account.get("password", "")).strip():
        alt = dict(account)
        alt["password"] = last_pwd
        attempts.append(alt)

    xor_options = [bool(cfg.get("use_xor_passwd", True))]
    if True not in xor_options:
        xor_options.append(True)
    if False not in xor_options:
        xor_options.append(False)

    last_err: Exception = RuntimeError("unknown")
    for use_xor in xor_options:
        cfg["use_xor_passwd"] = use_xor
        for candidate in attempts:
            try:
                return connect_chat_client(cfg, candidate, http_timeout, tcp_timeout, trace_id, label)
            except RuntimeError as e:
                last_err = e
                if "body={'error': 1009" in str(e):
                    continue
                raise
    raise last_err


def assert_history_contains(history_rsp: Dict, msg_id: str, kind: str) -> None:
    messages = list(history_rsp.get("messages") or [])
    if not any(str(one.get("msgid", "")) == msg_id for one in messages):
        raise RuntimeError(f"{kind} history does not contain probe message msgid={msg_id}, rsp={history_rsp}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify MemoChat offline private/group history flow")
    parser.add_argument("--config", default=rf"{REPO_ROOT}\local-loadtest\config.json", help="Path to loadtest config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--timeout", type=float, default=10.0, help="HTTP/TCP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    account_a, account_b = ensure_working_accounts(cfg, runtime_csv, args.timeout)

    client_a = None
    client_b = None
    result: Dict = {"ok": False}
    try:
        # A online (some runtime csv rows keep a rotated password in last_password)
        client_a, info_a = connect_with_password_fallback(
            cfg, account_a, args.timeout, args.timeout, "verify-offline-a", "verify-offline-a"
        )
        uid_a = int(info_a["uid"])

        # Bring B online briefly to learn uid/user_id.
        client_b, info_b0 = connect_with_password_fallback(
            cfg, account_b, args.timeout, args.timeout, "verify-offline-b0", "verify-offline-b0"
        )
        uid_b = int(info_b0["uid"])
        user_id_b = str(info_b0.get("user_id", "") or account_b.get("user_id", "") or "").strip()

        # Ensure they are friends so group create path is allowed.
        try:
            establish_friendship(client_a, client_b, info_a, info_b0, args.timeout)
        except Exception:
            # Friendship may already exist; ignore if the flow races with existing state.
            pass

        # Now close B to simulate offline delivery.
        client_b.close()
        client_b = None

        # Private: send while B offline
        private_payload = build_private_text_payload(uid_a, uid_b, "verify-offline-private")
        private_msg_id = str(private_payload["text_array"][0]["msgid"])
        client_a.send_json(ID_TEXT_CHAT_MSG_REQ, private_payload)
        _, private_rsp = client_a.recv_until([ID_TEXT_CHAT_MSG_RSP], args.timeout)
        private_rsp_status = expect_response_status(private_rsp)

        # Now B comes online, should be able to pull history containing A's message.
        client_b, info_b = connect_with_password_fallback(
            cfg, account_b, args.timeout, args.timeout, "verify-offline-b", "verify-offline-b"
        )
        client_b.send_json(ID_PRIVATE_HISTORY_REQ, {"fromuid": int(info_b["uid"]), "peer_uid": uid_a, "before_ts": 0, "limit": 50})
        _, private_history_rsp = client_b.recv_until([ID_PRIVATE_HISTORY_RSP], args.timeout)
        assert_history_contains(private_history_rsp, private_msg_id, "private")

        # Group: create group while both online (create requires friendship), then take B offline and send group msg.
        client_a.send_json(
            ID_CREATE_GROUP_REQ,
            build_group_create_payload(uid_a, "verify-offline-group", [user_id_b]),
        )
        _, create_rsp = client_a.recv_until([ID_CREATE_GROUP_RSP], args.timeout)
        group_id = int(create_rsp.get("groupid", 0) or 0)
        if group_id <= 0:
            raise RuntimeError(f"group create failed: {create_rsp}")

        client_b.close()
        client_b = None

        group_payload = build_group_message_payload(uid_a, group_id, "verify-offline-group-message")
        group_msg_id = str(group_payload["msg"]["msgid"])
        client_a.send_json(ID_GROUP_CHAT_MSG_REQ, group_payload)
        _, group_rsp = client_a.recv_until([ID_GROUP_CHAT_MSG_RSP], args.timeout)
        group_rsp_status = expect_response_status(group_rsp)

        # B back online: group history should contain the message.
        client_b, info_b2 = connect_with_password_fallback(
            cfg, account_b, args.timeout, args.timeout, "verify-offline-b2", "verify-offline-b2"
        )
        client_b.send_json(ID_GROUP_HISTORY_REQ, {"fromuid": int(info_b2["uid"]), "groupid": group_id, "before_ts": 0, "before_seq": 0, "limit": 50})
        _, group_history_rsp = client_b.recv_until([ID_GROUP_HISTORY_RSP], args.timeout)
        assert_history_contains(group_history_rsp, group_msg_id, "group")

        result = {
            "ok": True,
            "accounts": {"sender_uid": uid_a, "peer_uid": uid_b},
            "private": {
                "client_msg_id": private_msg_id,
                "response_status": private_rsp_status,
                "history_count": len(list(private_history_rsp.get("messages") or [])),
            },
            "group": {
                "group_id": group_id,
                "client_msg_id": group_msg_id,
                "response_status": group_rsp_status,
                "history_count": len(list(group_history_rsp.get("messages") or [])),
            },
        }
        print(compact(result))
        return 0
    finally:
        if client_a:
            client_a.close()
        if client_b:
            client_b.close()
        if not result.get("ok"):
            print(compact(result))


if __name__ == "__main__":
    raise SystemExit(main())

