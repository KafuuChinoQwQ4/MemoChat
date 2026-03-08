#!/usr/bin/env python3
import argparse
import json
import socket
import struct
import sys
import time
import urllib.error
import urllib.request
import uuid


ID_CHAT_LOGIN_REQ = 1005
ID_CHAT_LOGIN_RSP = 1006
ID_TEXT_CHAT_MSG_REQ = 1017
ID_TEXT_CHAT_MSG_RSP = 1018
ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019


def log(message):
    print(f"[INFO] {message}", flush=True)


def fail(message):
    print(f"[FAIL] {message}", flush=True)
    raise SystemExit(1)


def http_post_json(url, payload, timeout):
    body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        raw = resp.read().decode("utf-8")
    return json.loads(raw)


def normalize_host(host):
    host = (host or "").strip()
    if host in ("", "0.0.0.0", "::", "::1"):
        return "127.0.0.1"
    return host


def read_exact(sock, size):
    buf = bytearray()
    while len(buf) < size:
        chunk = sock.recv(size - len(buf))
        if not chunk:
            raise ConnectionError("socket closed while reading")
        buf.extend(chunk)
    return bytes(buf)


class ChatClient:
    def __init__(self, label, host, port, uid, token, timeout):
        self.label = label
        self.host = normalize_host(host)
        self.port = int(port)
        self.uid = int(uid)
        self.token = token
        self.timeout = timeout
        self.sock = None

    def connect(self):
        self.sock = socket.create_connection((self.host, self.port), timeout=self.timeout)
        self.sock.settimeout(self.timeout)
        payload = {
            "uid": self.uid,
            "token": self.token,
            "protocol_version": 2,
            "trace_id": f"smoke-{self.label}-{uuid.uuid4().hex[:12]}",
        }
        self.send_json(ID_CHAT_LOGIN_REQ, payload)
        msg_id, body = self.recv_until({ID_CHAT_LOGIN_RSP}, overall_timeout=self.timeout)
        if msg_id != ID_CHAT_LOGIN_RSP:
            fail(f"{self.label} login expected {ID_CHAT_LOGIN_RSP}, got {msg_id}")
        if body.get("error") != 0:
            fail(f"{self.label} chat login failed: {body}")
        log(f"{self.label} chat login ok on {self.host}:{self.port}")

    def close(self):
        if self.sock is None:
            return
        try:
            self.sock.close()
        finally:
            self.sock = None

    def send_json(self, msg_id, payload):
        data = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        header = struct.pack("!HH", int(msg_id), len(data))
        self.sock.sendall(header + data)

    def recv_frame(self):
        header = read_exact(self.sock, 4)
        msg_id, body_len = struct.unpack("!HH", header)
        body_raw = read_exact(self.sock, body_len)
        try:
            body = json.loads(body_raw.decode("utf-8"))
        except json.JSONDecodeError as exc:
            raise ValueError(f"{self.label} invalid json for msg {msg_id}: {exc}") from exc
        return msg_id, body

    def recv_until(self, expected_ids, overall_timeout):
        deadline = time.time() + overall_timeout
        while time.time() < deadline:
            remaining = max(0.1, deadline - time.time())
            self.sock.settimeout(remaining)
            msg_id, body = self.recv_frame()
            if msg_id in expected_ids:
                return msg_id, body
            log(f"{self.label} ignoring msg {msg_id}: {body}")
        raise TimeoutError(f"{self.label} timed out waiting for {sorted(expected_ids)}")


def login_via_gate(base_url, email, passwd, timeout):
    url = base_url.rstrip("/") + "/user_login"
    payload = {
        "email": email,
        "passwd": passwd,
        "client_ver": "2.0.0",
    }
    body = http_post_json(url, payload, timeout=timeout)
    if body.get("error") != 0:
        fail(f"HTTP login failed for {email}: {body}")
    required = ("uid", "token", "host", "port")
    missing = [key for key in required if key not in body]
    if missing:
        fail(f"HTTP login response missing {missing}: {body}")
    return body


def assert_text_packet(packet, expected_from, expected_to, expected_text, label):
    if packet.get("error") != 0:
        fail(f"{label} packet error: {packet}")
    if int(packet.get("fromuid", 0)) != expected_from:
        fail(f"{label} fromuid mismatch: {packet}")
    if int(packet.get("touid", 0)) != expected_to:
        fail(f"{label} touid mismatch: {packet}")
    text_array = packet.get("text_array") or []
    if not text_array:
        fail(f"{label} missing text_array: {packet}")
    content = text_array[0].get("content")
    if content != expected_text:
        fail(f"{label} content mismatch, expected {expected_text!r}, got {content!r}")


def run_smoke(args):
    login_a = login_via_gate(args.gate, args.email_a, args.pass_a, args.timeout)
    login_b = login_via_gate(args.gate, args.email_b, args.pass_b, args.timeout)
    log(
        "HTTP login routes: "
        f"A(uid={login_a['uid']}) -> {login_a['host']}:{login_a['port']}, "
        f"B(uid={login_b['uid']}) -> {login_b['host']}:{login_b['port']}"
    )
    if str(login_a["port"]) == str(login_b["port"]):
        fail(
            "Expected different chat ports for the two accounts, "
            f"but both logins returned {login_a['port']}"
        )

    client_a = ChatClient("A", login_a["host"], login_a["port"], login_a["uid"], login_a["token"], args.timeout)
    client_b = ChatClient("B", login_b["host"], login_b["port"], login_b["uid"], login_b["token"], args.timeout)
    try:
        client_a.connect()
        client_b.connect()

        text_ab = f"smoke-a-to-b-{uuid.uuid4().hex[:8]}"
        payload_ab = {
            "fromuid": int(login_a["uid"]),
            "touid": int(login_b["uid"]),
            "text_array": [
                {
                    "content": text_ab,
                    "msgid": f"smoke-{uuid.uuid4().hex}",
                    "created_at": int(time.time() * 1000),
                }
            ],
        }
        client_a.send_json(ID_TEXT_CHAT_MSG_REQ, payload_ab)
        msg_id, body = client_a.recv_until({ID_TEXT_CHAT_MSG_RSP}, overall_timeout=args.timeout)
        if msg_id != ID_TEXT_CHAT_MSG_RSP or body.get("error") != 0:
            fail(f"A send response invalid: msg_id={msg_id}, body={body}")
        msg_id, body = client_b.recv_until({ID_NOTIFY_TEXT_CHAT_MSG_REQ}, overall_timeout=args.timeout)
        if msg_id != ID_NOTIFY_TEXT_CHAT_MSG_REQ:
            fail(f"B notify expected {ID_NOTIFY_TEXT_CHAT_MSG_REQ}, got {msg_id}")
        assert_text_packet(body, int(login_a["uid"]), int(login_b["uid"]), text_ab, "A->B notify")
        log("A -> B private text delivery ok")

        text_ba = f"smoke-b-to-a-{uuid.uuid4().hex[:8]}"
        payload_ba = {
            "fromuid": int(login_b["uid"]),
            "touid": int(login_a["uid"]),
            "text_array": [
                {
                    "content": text_ba,
                    "msgid": f"smoke-{uuid.uuid4().hex}",
                    "created_at": int(time.time() * 1000),
                }
            ],
        }
        client_b.send_json(ID_TEXT_CHAT_MSG_REQ, payload_ba)
        msg_id, body = client_b.recv_until({ID_TEXT_CHAT_MSG_RSP}, overall_timeout=args.timeout)
        if msg_id != ID_TEXT_CHAT_MSG_RSP or body.get("error") != 0:
            fail(f"B send response invalid: msg_id={msg_id}, body={body}")
        msg_id, body = client_a.recv_until({ID_NOTIFY_TEXT_CHAT_MSG_REQ}, overall_timeout=args.timeout)
        if msg_id != ID_NOTIFY_TEXT_CHAT_MSG_REQ:
            fail(f"A notify expected {ID_NOTIFY_TEXT_CHAT_MSG_REQ}, got {msg_id}")
        assert_text_packet(body, int(login_b["uid"]), int(login_a["uid"]), text_ba, "B->A notify")
        log("B -> A private text delivery ok")
    finally:
        client_a.close()
        client_b.close()

    print("[PASS] dual-login routing and bidirectional private chat smoke test passed", flush=True)


def parse_args():
    parser = argparse.ArgumentParser(description="MemoChat dual-account private chat smoke test")
    parser.add_argument("--gate", default="http://127.0.0.1:8080", help="GateServer base URL")
    parser.add_argument("--timeout", type=float, default=10.0, help="Network timeout in seconds")
    parser.add_argument("--email-a", default="1220743777@qq.com", help="Account A email")
    parser.add_argument("--pass-a", default="745230", help="Account A password")
    parser.add_argument("--email-b", default="18165031775@qq.com", help="Account B email")
    parser.add_argument("--pass-b", default="123456", help="Account B password")
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        run_smoke(args)
    except urllib.error.URLError as exc:
        fail(f"HTTP request failed: {exc}")
    except (OSError, ValueError, TimeoutError, ConnectionError) as exc:
        fail(str(exc))


if __name__ == "__main__":
    main()
