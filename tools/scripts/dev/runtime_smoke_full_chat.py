#!/usr/bin/env python3
"""End-to-end MemoChat runtime smoke for login, dialogs, chat, and media.

The probe intentionally uses only the Python standard library plus the existing
Docker Postgres/Redis containers. It prepares a tiny stable data set, logs two
users into GateServer + ChatServer, then verifies private chat, group chat,
history, media upload/download, and media message delivery.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import os
import socket
import struct
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
from dataclasses import dataclass
from typing import Any, Callable

MSG_CHAT_LOGIN = 1005
MSG_CHAT_LOGIN_RSP = 1006
ID_TEXT_CHAT_MSG_REQ = 1017
ID_TEXT_CHAT_MSG_RSP = 1018
ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019
ID_GROUP_CHAT_MSG_REQ = 1044
ID_GROUP_CHAT_MSG_RSP = 1045
ID_NOTIFY_GROUP_CHAT_MSG_REQ = 1046
ID_GROUP_HISTORY_REQ = 1047
ID_GROUP_HISTORY_RSP = 1048
ID_PRIVATE_HISTORY_REQ = 1059
ID_PRIVATE_HISTORY_RSP = 1060
ID_GET_DIALOG_LIST_REQ = 1063
ID_GET_DIALOG_LIST_RSP = 1064
ID_GET_RELATION_BOOTSTRAP_REQ = 1092
ID_GET_RELATION_BOOTSTRAP_RSP = 1093


PNG_1X1 = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/p9sAAAAASUVORK5CYII="
)


@dataclass
class Account:
    uid: int
    email: str
    raw_password: str
    name: str
    user_id: str


@dataclass
class ChatSession:
    account: Account
    token: str
    login_ticket: str
    host: str
    port: int
    sock: socket.socket


def now_ms() -> int:
    return int(time.time() * 1000)


def xor_encode(raw: str) -> str:
    key = len(raw) % 255
    return "".join(chr(ord(ch) ^ key) for ch in raw)


def sql_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def run_command(args: list[str], *, input_text: str | None = None, timeout: float = 15.0) -> str:
    completed = subprocess.run(
        args,
        input=input_text,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed rc={completed.returncode}: {' '.join(args)}\n"
            f"stdout={completed.stdout.strip()}\nstderr={completed.stderr.strip()}"
        )
    return completed.stdout


def psql(container: str, sql: str, timeout: float = 15.0, db: str = "memo_pg") -> str:
    # After the gateserver microservice DB split, the account aggregate
    # (user/user_id) lives in memo_account and media metadata in memo_media,
    # while the chat domain (friend/chat_group/chat_*) stays in memo_pg. Route
    # each query to its owning database via the db argument.
    return run_command(
        ["docker", "exec", "-i", container, "psql", "-U", "memochat", "-d", db, "-At", "-v", "ON_ERROR_STOP=1"],
        input_text=sql,
        timeout=timeout,
    )


def redis_del(container: str, keys: list[str], timeout: float = 8.0) -> None:
    if not keys:
        return
    run_command(["docker", "exec", container, "redis-cli", "-a", "123456", "DEL", *keys], timeout=timeout)


def media_asset_row(args: argparse.Namespace, media_key: str) -> dict[str, Any]:
    sql = f"""
SET search_path TO memo, public;
SELECT media_key, owner_uid, media_type, origin_file_name, mime, size_bytes,
       storage_provider, storage_path, status
FROM chat_media_asset
WHERE media_key = {sql_quote(media_key)}
LIMIT 1;
"""
    output = psql(args.postgres_container, sql, timeout=args.timeout, db=args.media_db)
    rows = [line.strip() for line in output.splitlines() if line.strip() and "|" in line]
    if not rows:
        raise RuntimeError(f"media asset metadata missing in postgres for {media_key}: {output!r}")
    parts = rows[-1].split("|")
    if len(parts) != 9:
        raise RuntimeError(f"unexpected media asset metadata shape for {media_key}: {rows[-1]!r}")
    return {
        "media_key": parts[0],
        "owner_uid": int(parts[1]),
        "media_type": parts[2],
        "file_name": parts[3],
        "mime": parts[4],
        "size": int(parts[5]),
        "storage_provider": parts[6],
        "storage_path": parts[7],
        "status": int(parts[8]),
    }


def ensure_minio_alias(args: argparse.Namespace) -> None:
    run_command(
        [
            "docker",
            "exec",
            args.minio_container,
            "mc",
            "alias",
            "set",
            "local",
            args.minio_endpoint,
            args.minio_access_key,
            args.minio_secret_key,
        ],
        timeout=args.timeout,
    )


def minio_object_size(args: argparse.Namespace, storage_path: str) -> int:
    output = run_command(
        ["docker", "exec", args.minio_container, "mc", "stat", "--json", f"local/{storage_path}"],
        timeout=args.timeout,
    )
    stat = json.loads(output)
    return int(stat.get("size", -1))


def verify_uploaded_storage(
    args: argparse.Namespace,
    *,
    owner: ChatSession,
    uploaded: dict[str, Any],
    expected_type: str,
    expected_payload_size: int,
    expected_file_name: str,
    expected_mime: str,
) -> dict[str, Any]:
    media_key = str(uploaded.get("media_key", ""))
    row = media_asset_row(args, media_key)
    if row["media_key"] != media_key:
        raise RuntimeError(f"postgres media_key mismatch for {media_key}: {row}")
    if row["owner_uid"] != owner.account.uid:
        raise RuntimeError(f"postgres owner mismatch for {media_key}: {row}")
    if row["media_type"] != expected_type:
        raise RuntimeError(f"postgres media_type mismatch for {media_key}: {row}")
    if row["file_name"] != expected_file_name:
        raise RuntimeError(f"postgres file name mismatch for {media_key}: {row}")
    if row["mime"] != expected_mime:
        raise RuntimeError(f"postgres mime mismatch for {media_key}: {row}")
    if row["size"] != expected_payload_size or int(uploaded.get("size", 0)) != expected_payload_size:
        raise RuntimeError(f"media size mismatch for {media_key}: row={row} uploaded={uploaded}")
    if row["status"] != 1 or row["storage_provider"] != "s3":
        raise RuntimeError(f"media storage metadata mismatch for {media_key}: {row}")
    object_size = minio_object_size(args, row["storage_path"])
    if object_size != expected_payload_size:
        raise RuntimeError(
            f"minio object size mismatch for {media_key}: storage_path={row['storage_path']} "
            f"size={object_size} expected={expected_payload_size}"
        )
    return {
        "postgres": row,
        "minio_object_size": object_size,
    }


def prepare_data(args: argparse.Namespace, accounts: tuple[Account, Account]) -> int:
    a, b = accounts
    password_hash = xor_encode(a.raw_password)
    # The account aggregate (user/user_id) was split into memo_account by the
    # gateserver microservice DB split; login/account services read it there.
    # Seed users into memo_account so the smoke exercises the same data the
    # services actually read, not the retired memo_pg copy.
    user_sql = f"""
SET search_path TO memo, public;
BEGIN;
INSERT INTO "user" (uid, name, email, pwd, nick, "desc", sex, icon, user_id)
VALUES
  ({a.uid}, {sql_quote(a.name)}, {sql_quote(a.email)}, {sql_quote(password_hash)}, {sql_quote(a.name)}, 'runtime smoke user', 0, '', {sql_quote(a.user_id)}),
  ({b.uid}, {sql_quote(b.name)}, {sql_quote(b.email)}, {sql_quote(password_hash)}, {sql_quote(b.name)}, 'runtime smoke user', 0, '', {sql_quote(b.user_id)})
ON CONFLICT (uid) DO UPDATE SET
  name = EXCLUDED.name,
  email = EXCLUDED.email,
  pwd = EXCLUDED.pwd,
  nick = EXCLUDED.nick,
  "desc" = EXCLUDED."desc",
  sex = EXCLUDED.sex,
  icon = EXCLUDED.icon,
  user_id = EXCLUDED.user_id;
COMMIT;
"""
    psql(args.postgres_container, user_sql, db=args.account_db)

    # Chat-domain relations (friend/group/membership) stay in memo_pg.
    setup_sql = f"""
SET search_path TO memo, public;
BEGIN;
INSERT INTO friend (self_id, friend_id, back)
VALUES
  ({a.uid}, {b.uid}, {sql_quote(b.name)}),
  ({b.uid}, {a.uid}, {sql_quote(a.name)})
ON CONFLICT (self_id, friend_id) DO UPDATE SET back = EXCLUDED.back;

WITH upsert_group AS (
  INSERT INTO chat_group (group_code, name, owner_uid, announcement, member_limit, status)
  VALUES ({sql_quote(args.group_code)}, 'Runtime Smoke Full Chat', {a.uid}, 'runtime smoke group', 20, 1)
  ON CONFLICT (group_code) DO UPDATE SET
    name = EXCLUDED.name,
    owner_uid = EXCLUDED.owner_uid,
    announcement = EXCLUDED.announcement,
    member_limit = EXCLUDED.member_limit,
    status = 1,
    updated_at = CURRENT_TIMESTAMP
  RETURNING group_id
)
SELECT group_id FROM upsert_group
UNION ALL
SELECT group_id FROM chat_group WHERE group_code = {sql_quote(args.group_code)}
LIMIT 1;
COMMIT;
"""
    output = psql(args.postgres_container, setup_sql)
    group_ids = [int(line.strip()) for line in output.splitlines() if line.strip().isdigit()]
    if not group_ids:
        raise RuntimeError(f"could not resolve smoke group id from psql output: {output!r}")
    group_id = group_ids[-1]
    membership_sql = f"""
SET search_path TO memo, public;
INSERT INTO chat_group_member (group_id, uid, role, mute_until, join_source, status)
VALUES
  ({group_id}, {a.uid}, 3, 0, 0, 1),
  ({group_id}, {b.uid}, 1, 0, 0, 1)
ON CONFLICT (group_id, uid) DO UPDATE SET
  role = EXCLUDED.role,
  mute_until = 0,
  status = 1,
  updated_at = CURRENT_TIMESTAMP;
"""
    psql(args.postgres_container, membership_sql)
    redis_del(
        args.redis_container,
        [
            f"relation_bootstrap_{a.uid}",
            f"relation_bootstrap_{b.uid}",
            f"ubaseinfo_{a.uid}",
            f"ubaseinfo_{b.uid}",
            f"uip_{a.uid}",
            f"uip_{b.uid}",
            f"usession_{a.uid}",
            f"usession_{b.uid}",
        ],
    )
    return group_id


def post_json(url: str, payload: dict[str, Any], timeout: float) -> dict[str, Any]:
    body = json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(url, data=body, headers={"Content-Type": "application/json"}, method="POST")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            text = response.read().decode("utf-8", errors="replace")
            return json.loads(text) if text else {}
    except urllib.error.HTTPError as exc:
        text = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"POST {url} failed http={exc.code} body={text}") from exc


def get_bytes(url: str, timeout: float) -> tuple[int, bytes, str]:
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            return response.status, response.read(), response.headers.get("Content-Type", "")
    except urllib.error.HTTPError as exc:
        return exc.code, exc.read(), exc.headers.get("Content-Type", "")


def get_json(url: str, timeout: float) -> dict[str, Any]:
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            text = response.read().decode("utf-8", errors="replace")
            return json.loads(text) if text else {}
    except urllib.error.HTTPError as exc:
        text = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"GET {url} failed http={exc.code} body={text}") from exc


def post_binary(url: str, payload: bytes, headers: dict[str, str], timeout: float) -> dict[str, Any]:
    req = urllib.request.Request(
        url,
        data=payload,
        headers={"Content-Type": "application/octet-stream", **headers},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            text = response.read().decode("utf-8", errors="replace")
            return json.loads(text) if text else {}
    except urllib.error.HTTPError as exc:
        text = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"POST {url} failed http={exc.code} body={text}") from exc


def encode_frame(msg_id: int, payload: dict[str, Any]) -> bytes:
    body = json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    if len(body) > 0xFFFF:
        raise ValueError("payload too large")
    return struct.pack("!HH", msg_id, len(body)) + body


def recv_exact(sock: socket.socket, size: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < size:
        chunk = sock.recv(size - len(chunks))
        if not chunk:
            raise ConnectionError("socket closed")
        chunks.extend(chunk)
    return bytes(chunks)


def recv_frame(sock: socket.socket) -> tuple[int, dict[str, Any]]:
    header = recv_exact(sock, 4)
    msg_id, size = struct.unpack("!HH", header)
    payload = recv_exact(sock, size)
    if not payload:
        return msg_id, {}
    return msg_id, json.loads(payload.decode("utf-8", errors="replace"))


def recv_matching(
    sock: socket.socket,
    expected_ids: set[int],
    predicate: Callable[[int, dict[str, Any]], bool],
    timeout: float,
    stage: str,
) -> tuple[int, dict[str, Any], list[tuple[int, dict[str, Any]]]]:
    deadline = time.monotonic() + timeout
    skipped: list[tuple[int, dict[str, Any]]] = []
    while time.monotonic() < deadline:
        sock.settimeout(max(0.05, deadline - time.monotonic()))
        try:
            msg_id, payload = recv_frame(sock)
        except socket.timeout as exc:
            raise TimeoutError(
                f"{stage}: socket timed out waiting for ids={sorted(expected_ids)}, skipped={skipped[-5:]}"
            ) from exc
        if msg_id in expected_ids and predicate(msg_id, payload):
            return msg_id, payload, skipped
        skipped.append((msg_id, payload))
    raise TimeoutError(f"{stage}: timed out waiting for ids={sorted(expected_ids)}, skipped={skipped[-5:]}")


def send_request(
    session: ChatSession,
    request_id: int,
    response_id: int,
    payload: dict[str, Any],
    timeout: float,
    stage: str,
    predicate: Callable[[dict[str, Any]], bool] | None = None,
) -> dict[str, Any]:
    payload.setdefault("trace_id", uuid.uuid4().hex)
    payload.setdefault("protocol_version", 3)
    session.sock.sendall(encode_frame(request_id, payload))
    _, response, _ = recv_matching(
        session.sock,
        {response_id},
        lambda _msg_id, body: int(body.get("error", 0)) == 0 and (predicate(body) if predicate else True),
        timeout,
        stage,
    )
    return response


def gate_login(args: argparse.Namespace, account: Account) -> dict[str, Any]:
    payload = {
        "email": account.email,
        "passwd": xor_encode(account.raw_password),
        "client_ver": "3.0.0",
    }
    response = post_json(args.gate_url.rstrip("/") + "/user_login", payload, args.timeout)
    if int(response.get("error", -1)) != 0:
        raise RuntimeError(f"gate login failed for {account.email}: {response}")
    if int(response.get("uid", 0)) != account.uid:
        raise RuntimeError(f"gate login returned unexpected uid for {account.email}: {response}")
    return response


def normalize_host(host: Any) -> str:
    value = str(host or "127.0.0.1").strip() or "127.0.0.1"
    if value in {"0.0.0.0", "::", "::1", "localhost"}:
        return "127.0.0.1"
    return value


def open_chat_session(args: argparse.Namespace, account: Account) -> ChatSession:
    gate = gate_login(args, account)
    host = normalize_host(gate.get("host") or gate.get("tcp_host"))
    port = int(gate.get("port") or gate.get("tcp_port") or 0)
    if port <= 0:
        raise RuntimeError(f"gate response did not include a chat port: {gate}")
    sock = socket.create_connection((host, port), timeout=args.timeout)
    sock.settimeout(args.timeout)
    payload = {
        "uid": account.uid,
        "token": gate.get("token", ""),
        "login_ticket": gate.get("login_ticket", ""),
        "ticket_expire_ms": gate.get("ticket_expire_ms", 0),
        "client_ver": "3.0.0",
        "protocol_version": 3,
        "trace_id": uuid.uuid4().hex,
    }
    sock.sendall(encode_frame(MSG_CHAT_LOGIN, payload))
    _, response, _ = recv_matching(
        sock,
        {MSG_CHAT_LOGIN_RSP},
        lambda _msg_id, body: int(body.get("error", -1)) == 0 and int(body.get("uid", 0)) == account.uid,
        args.timeout,
        f"chat_login:{account.uid}",
    )
    return ChatSession(
        account=account,
        token=str(gate.get("token", "")),
        login_ticket=str(gate.get("login_ticket", "")),
        host=host,
        port=port,
        sock=sock,
    )


def text_array_has(payload: dict[str, Any], msg_id: str, content: str) -> bool:
    for item in payload.get("text_array", []) or []:
        if item.get("msgid") == msg_id and item.get("content") == content:
            return True
    return False


def history_has_message(payload: dict[str, Any], msg_id: str, content: str) -> bool:
    for item in payload.get("messages", []) or []:
        if item.get("msgid") == msg_id and item.get("content") == content:
            return True
    return False


def dialog_list_has(dialogs: list[Any], *, peer_uid: int | None = None, group_id: int | None = None) -> bool:
    for item in dialogs:
        if not isinstance(item, dict):
            continue
        if peer_uid is not None and item.get("dialog_type") == "private" and int(item.get("peer_uid", 0)) == peer_uid:
            return True
        if group_id is not None and item.get("dialog_type") == "group" and int(item.get("group_id", 0)) == group_id:
            return True
    return False


def ensure_bootstrap(session: ChatSession, peer_uid: int, timeout: float) -> dict[str, Any]:
    payload = send_request(
        session,
        ID_GET_RELATION_BOOTSTRAP_REQ,
        ID_GET_RELATION_BOOTSTRAP_RSP,
        {"fromuid": session.account.uid},
        timeout,
        f"relation_bootstrap:{session.account.uid}",
    )
    friends = payload.get("friend_list", []) or []
    if not any(isinstance(item, dict) and int(item.get("uid", 0)) == peer_uid for item in friends):
        raise RuntimeError(f"relation bootstrap missing peer {peer_uid}: {payload}")
    return payload


def ensure_dialogs(session: ChatSession, peer_uid: int, group_id: int, timeout: float) -> dict[str, Any]:
    payload = send_request(
        session,
        ID_GET_DIALOG_LIST_REQ,
        ID_GET_DIALOG_LIST_RSP,
        {"fromuid": session.account.uid},
        timeout,
        f"dialog_list:{session.account.uid}",
    )
    dialogs = payload.get("dialogs", []) or []
    if not dialog_list_has(dialogs, peer_uid=peer_uid):
        raise RuntimeError(f"dialog list missing private peer {peer_uid}: {payload}")
    if not dialog_list_has(dialogs, group_id=group_id):
        raise RuntimeError(f"dialog list missing group {group_id}: {payload}")
    return payload


def send_private(
    sender: ChatSession, receiver: ChatSession, content: str, msg_id: str, timeout: float
) -> dict[str, Any]:
    payload = {
        "fromuid": sender.account.uid,
        "touid": receiver.account.uid,
        "text_array": [{"msgid": msg_id, "content": content, "created_at": now_ms()}],
    }
    ack = send_request(
        sender,
        ID_TEXT_CHAT_MSG_REQ,
        ID_TEXT_CHAT_MSG_RSP,
        payload,
        timeout,
        f"private_send:{msg_id}",
        lambda body: body.get("client_msg_id") == msg_id and text_array_has(body, msg_id, content),
    )
    recv_matching(
        receiver.sock,
        {ID_NOTIFY_TEXT_CHAT_MSG_REQ},
        lambda _msg_id, body: (
            int(body.get("fromuid", 0)) == sender.account.uid
            and int(body.get("touid", 0)) == receiver.account.uid
            and text_array_has(body, msg_id, content)
        ),
        timeout,
        f"private_notify:{msg_id}",
    )
    history = send_request(
        receiver,
        ID_PRIVATE_HISTORY_REQ,
        ID_PRIVATE_HISTORY_RSP,
        {
            "fromuid": receiver.account.uid,
            "peer_uid": sender.account.uid,
            "before_ts": 0,
            "before_msg_id": "",
            "limit": 20,
        },
        timeout,
        f"private_history:{msg_id}",
        lambda body: history_has_message(body, msg_id, content),
    )
    return {"ack": ack, "history_count": len(history.get("messages", []) or [])}


def send_group(
    sender: ChatSession, receiver: ChatSession, group_id: int, content: str, msg_id: str, timeout: float
) -> dict[str, Any]:
    msg_type = "image" if content.startswith("__memochat_img__:") else "text"
    payload = {
        "fromuid": sender.account.uid,
        "groupid": group_id,
        "msg": {
            "msgid": msg_id,
            "content": content,
            "msgtype": msg_type,
            "mentions": [],
            "mention_all": False,
        },
    }
    ack = send_request(
        sender,
        ID_GROUP_CHAT_MSG_REQ,
        ID_GROUP_CHAT_MSG_RSP,
        payload,
        timeout,
        f"group_send:{msg_id}",
        lambda body: body.get("client_msg_id") == msg_id and int(body.get("groupid", 0)) == group_id,
    )
    recv_matching(
        receiver.sock,
        {ID_NOTIFY_GROUP_CHAT_MSG_REQ},
        lambda _msg_id, body: body.get("client_msg_id") == msg_id and int(body.get("groupid", 0)) == group_id,
        timeout,
        f"group_notify:{msg_id}",
    )
    history = send_request(
        receiver,
        ID_GROUP_HISTORY_REQ,
        ID_GROUP_HISTORY_RSP,
        {"fromuid": receiver.account.uid, "groupid": group_id, "before_ts": 0, "before_seq": 0, "limit": 20},
        timeout,
        f"group_history:{msg_id}",
        lambda body: history_has_message(body, msg_id, content),
    )
    return {"ack": ack, "history_count": len(history.get("messages", []) or [])}


def upload_media(args: argparse.Namespace, owner: ChatSession, run_id: str) -> dict[str, Any]:
    response = post_json(
        args.gate_url.rstrip("/") + "/upload_media",
        {
            "uid": owner.account.uid,
            "token": owner.token,
            "media_type": "image",
            "file_name": f"smoke-{run_id}.png",
            "mime": "image/png",
            "data_base64": base64.b64encode(PNG_1X1).decode("ascii"),
        },
        args.timeout,
    )
    if int(response.get("error", -1)) != 0:
        raise RuntimeError(f"media upload failed: {response}")
    media_key = str(response.get("media_key", ""))
    relative_url = str(response.get("url", ""))
    if not media_key or not relative_url:
        raise RuntimeError(f"media upload response missing key/url: {response}")
    download_url = (
        args.gate_url.rstrip()
        + relative_url
        + "&"
        + urllib.parse.urlencode({"uid": owner.account.uid, "token": owner.token})
    )
    status, data, content_type = get_bytes(download_url, args.timeout)
    if status != 200 or data != PNG_1X1:
        raise RuntimeError(
            f"media download mismatch status={status} content_type={content_type} size={len(data)} expected={len(PNG_1X1)}"
        )
    response["absolute_url"] = args.gate_url.rstrip("/") + relative_url
    response["download_content_type"] = content_type
    response["size"] = len(PNG_1X1)
    return response


def deterministic_payload(label: str, size: int) -> bytes:
    seed = label.encode("utf-8")
    out = bytearray()
    counter = 0
    while len(out) < size:
        out.extend(hashlib.sha256(seed + counter.to_bytes(4, "big")).digest())
        counter += 1
    return bytes(out[:size])


def upload_chunked_media_file(
    args: argparse.Namespace,
    owner: ChatSession,
    *,
    file_name: str,
    mime: str,
    payload: bytes,
) -> dict[str, Any]:
    init = post_json(
        args.gate_url.rstrip("/") + "/upload_media_init",
        {
            "uid": owner.account.uid,
            "token": owner.token,
            "media_type": "file",
            "file_name": file_name,
            "mime": mime,
            "file_size": len(payload),
        },
        args.timeout,
    )
    if int(init.get("error", -1)) != 0:
        raise RuntimeError(f"media chunk init failed for {file_name}: {init}")
    upload_id = str(init.get("upload_id", ""))
    chunk_size = int(init.get("chunk_size", 0))
    total_chunks = int(init.get("total_chunks", 0))
    if not upload_id or chunk_size <= 0 or total_chunks != math.ceil(len(payload) / chunk_size):
        raise RuntimeError(f"media chunk init returned invalid session for {file_name}: {init}")

    uploaded_sizes: list[int] = []
    for index in range(total_chunks):
        start = index * chunk_size
        chunk = payload[start : start + chunk_size]
        chunk_rsp = post_binary(
            args.gate_url.rstrip("/") + "/upload_media_chunk",
            chunk,
            {
                "X-Uid": str(owner.account.uid),
                "X-Token": owner.token,
                "X-Upload-Id": upload_id,
                "X-Chunk-Index": str(index),
            },
            args.timeout,
        )
        if int(chunk_rsp.get("error", -1)) != 0:
            raise RuntimeError(f"media chunk upload failed for {file_name} index={index}: {chunk_rsp}")
        uploaded_sizes.append(len(chunk))

    status_url = (
        args.gate_url.rstrip("/")
        + "/upload_media_status?"
        + urllib.parse.urlencode({"uid": owner.account.uid, "token": owner.token, "upload_id": upload_id})
    )
    status = get_json(status_url, args.timeout)
    if int(status.get("error", -1)) != 0:
        raise RuntimeError(f"media status failed for {file_name}: {status}")
    uploaded_indexes = sorted(int(v) for v in status.get("uploaded_chunks", []) or [])
    if uploaded_indexes != list(range(total_chunks)):
        raise RuntimeError(f"media status missing chunks for {file_name}: {status}")

    complete = post_json(
        args.gate_url.rstrip("/") + "/upload_media_complete",
        {"uid": owner.account.uid, "token": owner.token, "upload_id": upload_id},
        args.timeout,
    )
    if int(complete.get("error", -1)) != 0:
        raise RuntimeError(f"media complete failed for {file_name}: {complete}")
    relative_url = str(complete.get("url", ""))
    media_key = str(complete.get("media_key", ""))
    if not media_key or not relative_url:
        raise RuntimeError(f"media complete response missing key/url for {file_name}: {complete}")

    download_url = (
        args.gate_url.rstrip("/")
        + relative_url
        + "&"
        + urllib.parse.urlencode({"uid": owner.account.uid, "token": owner.token})
    )
    status_code, downloaded, content_type = get_bytes(download_url, args.timeout)
    if status_code != 200 or downloaded != payload:
        raise RuntimeError(
            f"media download mismatch for {file_name}: status={status_code} "
            f"content_type={content_type} size={len(downloaded)} expected={len(payload)}"
        )
    return {
        "media_key": media_key,
        "media_type": complete.get("media_type"),
        "file_name": complete.get("file_name"),
        "mime": complete.get("mime"),
        "size": int(complete.get("size", 0)),
        "url": relative_url,
        "absolute_url": args.gate_url.rstrip("/") + relative_url,
        "chunk_size": chunk_size,
        "total_chunks": total_chunks,
        "uploaded_chunk_sizes": uploaded_sizes,
        "sha256": hashlib.sha256(payload).hexdigest(),
        "download_content_type": content_type,
    }


def encode_file_marker(url: str, file_name: str, mime: str, size: int) -> str:
    payload = json.dumps(
        {"url": url, "name": file_name, "mime": mime, "size": size},
        separators=(",", ":"),
        ensure_ascii=False,
    ).encode("utf-8")
    return "__memochat_file__:json:" + base64.b64encode(payload).decode("ascii")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gate-url", default="http://127.0.0.1:80")
    parser.add_argument("--postgres-container", default="memochat-postgres")
    parser.add_argument(
        "--account-db", default="memo_account", help="database owning the user/user_id account aggregate"
    )
    parser.add_argument("--media-db", default="memo_media", help="database owning chat_media_asset metadata")
    parser.add_argument("--redis-container", default="memochat-redis")
    parser.add_argument("--minio-container", default="memochat-minio")
    parser.add_argument("--minio-endpoint", default="http://127.0.0.1:9000")
    parser.add_argument("--minio-access-key", default=os.environ.get("MEMOCHAT_MINIO_ACCESS_KEY", "memochat_admin"))
    parser.add_argument("--minio-secret-key", default=os.environ.get("MEMOCHAT_MINIO_SECRET_KEY", "MinioPass2026!"))
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--group-code", default="smkfull01")
    args = parser.parse_args(argv)

    run_id = uuid.uuid4().hex[:10]
    password = "Sm0ke123456"
    accounts = (
        Account(910001, "runtime.smoke.a@memochat.local", password, "RuntimeSmokeA", "smka001"),
        Account(910002, "runtime.smoke.b@memochat.local", password, "RuntimeSmokeB", "smkb002"),
    )
    started = time.time()
    sessions: list[ChatSession] = []
    summary: dict[str, Any] = {
        "run_id": run_id,
        "gate_url": args.gate_url,
        "checks": {},
        "accounts": [account.uid for account in accounts],
    }
    try:
        group_id = prepare_data(args, accounts)
        ensure_minio_alias(args)
        summary["group_id"] = group_id
        summary["checks"]["data_setup"] = "PASS"
        summary["checks"]["minio_alias"] = "PASS"

        session_a = open_chat_session(args, accounts[0])
        session_b = open_chat_session(args, accounts[1])
        sessions.extend([session_a, session_b])
        summary["checks"]["login"] = {
            "status": "PASS",
            "a_endpoint": f"{session_a.host}:{session_a.port}",
            "b_endpoint": f"{session_b.host}:{session_b.port}",
        }

        ensure_bootstrap(session_a, accounts[1].uid, args.timeout)
        ensure_bootstrap(session_b, accounts[0].uid, args.timeout)
        summary["checks"]["relation_bootstrap"] = "PASS"

        ensure_dialogs(session_a, accounts[1].uid, group_id, args.timeout)
        ensure_dialogs(session_b, accounts[0].uid, group_id, args.timeout)
        summary["checks"]["dialog_list"] = "PASS"

        private_text_id = f"smoke-private-{run_id}"
        private_text = f"runtime smoke private text {run_id}"
        summary["checks"]["private_text"] = send_private(
            session_a, session_b, private_text, private_text_id, args.timeout
        )

        group_text_id = f"smoke-group-{run_id}"
        group_text = f"runtime smoke group text {run_id}"
        summary["checks"]["group_text"] = send_group(
            session_a, session_b, group_id, group_text, group_text_id, args.timeout
        )

        media = upload_media(args, session_a, run_id)
        media_storage = verify_uploaded_storage(
            args,
            owner=session_a,
            uploaded=media,
            expected_type="image",
            expected_payload_size=len(PNG_1X1),
            expected_file_name=f"smoke-{run_id}.png",
            expected_mime="image/png",
        )
        summary["checks"]["media_upload_download"] = {
            "status": "PASS",
            "media_key": media.get("media_key"),
            "size": media.get("size"),
            "content_type": media.get("download_content_type"),
            "storage": media_storage,
        }

        media_content = "__memochat_img__:" + str(media["absolute_url"])
        private_media_id = f"smoke-private-media-{run_id}"
        summary["checks"]["private_media"] = send_private(
            session_a, session_b, media_content, private_media_id, args.timeout
        )

        group_media_id = f"smoke-group-media-{run_id}"
        summary["checks"]["group_media"] = send_group(
            session_a, session_b, group_id, media_content, group_media_id, args.timeout
        )

        file_specs = [
            (
                f"smoke-{run_id}.zip",
                "application/zip",
                deterministic_payload(f"{run_id}:zip", 393_537),
            ),
            (
                f"smoke-{run_id}.pdf",
                "application/pdf",
                deterministic_payload(f"{run_id}:pdf", 263_187),
            ),
            (
                f"smoke-{run_id}.docx",
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
                deterministic_payload(f"{run_id}:docx", 340_999),
            ),
            (
                f"smoke-{run_id}-multichunk.zip",
                "application/zip",
                deterministic_payload(f"{run_id}:multichunk", 5_243_213),
            ),
        ]
        summary["checks"]["file_upload_download"] = []
        summary["checks"]["private_file_messages"] = []
        for index, (file_name, mime, payload) in enumerate(file_specs):
            uploaded = upload_chunked_media_file(args, session_a, file_name=file_name, mime=mime, payload=payload)
            storage = verify_uploaded_storage(
                args,
                owner=session_a,
                uploaded=uploaded,
                expected_type="file",
                expected_payload_size=len(payload),
                expected_file_name=file_name,
                expected_mime=mime,
            )
            summary["checks"]["file_upload_download"].append(
                {
                    "status": "PASS",
                    "media_key": uploaded["media_key"],
                    "file_name": uploaded["file_name"],
                    "mime": uploaded["mime"],
                    "size": uploaded["size"],
                    "chunk_size": uploaded["chunk_size"],
                    "total_chunks": uploaded["total_chunks"],
                    "uploaded_chunk_sizes": uploaded["uploaded_chunk_sizes"],
                    "sha256": uploaded["sha256"],
                    "content_type": uploaded["download_content_type"],
                    "storage": storage,
                }
            )
            file_marker = encode_file_marker(
                str(uploaded["absolute_url"]),
                str(uploaded["file_name"]),
                str(uploaded["mime"]),
                int(uploaded["size"]),
            )
            file_msg_id = f"smoke-private-file-{index}-{run_id}"
            delivered = send_private(session_a, session_b, file_marker, file_msg_id, args.timeout)
            summary["checks"]["private_file_messages"].append(
                {
                    "status": "PASS",
                    "msg_id": file_msg_id,
                    "media_key": uploaded["media_key"],
                    "file_name": uploaded["file_name"],
                    "history_count": delivered["history_count"],
                }
            )

        summary["status"] = "PASS"
        summary["elapsed_sec"] = round(time.time() - started, 3)
        print(json.dumps(summary, ensure_ascii=False, indent=2, sort_keys=True))
        return 0
    except Exception as exc:
        summary["status"] = "FAIL"
        summary["error"] = str(exc)
        summary["elapsed_sec"] = round(time.time() - started, 3)
        print(json.dumps(summary, ensure_ascii=False, indent=2, sort_keys=True), file=sys.stderr)
        return 1
    finally:
        for session in sessions:
            try:
                session.sock.close()
            except Exception:
                pass


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
