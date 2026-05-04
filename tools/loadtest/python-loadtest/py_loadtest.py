#!/usr/bin/env python3
"""Python-only load test runner for MemoChat.

The runner intentionally uses only the Python standard library so it works on a
fresh Windows dev machine without rebuilding extra load-test binaries.
"""

from __future__ import annotations

import argparse
import base64
import csv
import json
import math
import socket
import statistics
import struct
import threading
import time
import urllib.error
import urllib.request
import uuid
import http.client
from urllib.parse import urlparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


MSG_CHAT_LOGIN = 1005
MSG_CHAT_LOGIN_RSP = 1006


_thread_local = threading.local()


@dataclass
class Account:
    email: str
    password: str
    uid: str = ""
    user_id: str = ""
    user: str = ""


@dataclass
class OperationResult:
    ok: bool
    elapsed_ms: float
    stage: str = "ok"
    error: str = ""
    error_code: int = 0
    data: dict[str, Any] = field(default_factory=dict)


@dataclass
class ScenarioResult:
    scenario: str
    total: int
    success: int
    failed: int
    latencies_ms: list[float]
    started_at: str
    finished_at: str
    wall_elapsed_sec: float = 0.0
    errors: dict[str, int] = field(default_factory=dict)
    quality: dict[str, float] = field(default_factory=dict)
    details: dict[str, Any] = field(default_factory=dict)


def now_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def resolve_path(path: str, base_dir: Path) -> Path:
    candidate = Path(path)
    if candidate.is_absolute():
        return candidate
    return (base_dir / candidate).resolve()


def load_accounts(path: Path, prefer_last_password: bool = False) -> list[Account]:
    accounts: list[Account] = []
    with path.open("r", newline="", encoding="utf-8-sig") as f:
        for row in csv.DictReader(f):
            email = (row.get("email") or "").strip()
            password = (row.get("password") or "").strip()
            last_password = (row.get("last_password") or "").strip()
            if prefer_last_password and last_password:
                password = last_password
            if email and password:
                accounts.append(
                    Account(
                        email=email,
                        password=password,
                        uid=(row.get("uid") or "").strip(),
                        user_id=(row.get("user_id") or "").strip(),
                        user=(row.get("user") or "").strip(),
                    )
                )
    return accounts


def xor_encode(raw: str) -> str:
    key = len(raw) % 255
    return "".join(chr(ord(ch) ^ key) for ch in raw)


def post_json(url: str, payload: dict[str, Any], timeout: float = 8.0) -> tuple[int, dict[str, Any], str]:
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=body,
        headers={
            "Content-Type": "application/json",
            "X-Trace-Id": uuid.uuid4().hex,
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            text = resp.read().decode("utf-8", errors="replace")
            try:
                data = json.loads(text) if text else {}
            except json.JSONDecodeError:
                data = {"raw": text}
            return resp.status, data, ""
    except urllib.error.HTTPError as exc:
        text = exc.read().decode("utf-8", errors="replace")
        try:
            data = json.loads(text) if text else {}
        except json.JSONDecodeError:
            data = {"raw": text}
        return exc.code, data, str(exc)
    except Exception as exc:
        return 0, {}, str(exc)


def thread_http_connection(parsed_url, timeout: float, keep_alive: bool) -> http.client.HTTPConnection:
    if not keep_alive:
        return http.client.HTTPConnection(parsed_url.hostname, parsed_url.port or 80, timeout=timeout)
    key = f"{parsed_url.scheme}://{parsed_url.netloc}"
    conns = getattr(_thread_local, "http_conns", None)
    if conns is None:
        conns = {}
        _thread_local.http_conns = conns
    conn = conns.get(key)
    if conn is None:
        conn = http.client.HTTPConnection(parsed_url.hostname, parsed_url.port or 80, timeout=timeout)
        conns[key] = conn
    return conn


def post_json_fast(url: str, payload: dict[str, Any], timeout: float = 8.0, keep_alive: bool = True) -> tuple[int, dict[str, Any], str]:
    parsed = urlparse(url)
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    path = parsed.path or "/"
    if parsed.query:
        path += "?" + parsed.query
    headers = {
        "Content-Type": "application/json",
        "Content-Length": str(len(body)),
        "X-Trace-Id": uuid.uuid4().hex,
        "Connection": "keep-alive" if keep_alive else "close",
    }
    try:
        conn = thread_http_connection(parsed, timeout, keep_alive)
        conn.request("POST", path, body=body, headers=headers)
        resp = conn.getresponse()
        text = resp.read().decode("utf-8", errors="replace")
        try:
            data = json.loads(text) if text else {}
        except json.JSONDecodeError:
            data = {"raw": text}
        if not keep_alive:
            conn.close()
        return resp.status, data, ""
    except Exception as exc:
        conns = getattr(_thread_local, "http_conns", None)
        if conns is not None:
            key = f"{parsed.scheme}://{parsed.netloc}"
            old = conns.pop(key, None)
            if old is not None:
                try:
                    old.close()
                except Exception:
                    pass
        return 0, {}, str(exc)


class GateUrlPicker:
    def __init__(self, urls: list[str]):
        self.urls = [url.rstrip("/") for url in urls if url.strip()]
        self._index = 0
        self._lock = threading.Lock()

    def next(self) -> str:
        if not self.urls:
            return "http://127.0.0.1:8080"
        with self._lock:
            value = self.urls[self._index % len(self.urls)]
            self._index += 1
            return value


def gate_urls_from_config(config: dict[str, Any]) -> list[str]:
    raw = config.get("gate_urls")
    if isinstance(raw, list) and raw:
        return [str(item) for item in raw]
    return [str(config.get("gate_url", "http://127.0.0.1:8080"))]


def gate_login(config: dict[str, Any], account: Account, gate_url: str | None = None) -> OperationResult:
    started = time.perf_counter()
    gate_url = (gate_url or config.get("gate_url", "http://127.0.0.1:8080")).rstrip("/")
    login_path = config.get("login_path", "/user_login")
    password = xor_encode(account.password) if config.get("use_xor_passwd", True) else account.password
    payload = {
        "email": account.email,
        "passwd": password,
        "client_ver": str(config.get("client_ver", "3.0.0")),
    }
    if config.get("fast_http", False):
        status, data, error = post_json_fast(
            f"{gate_url}{login_path}",
            payload,
            float(config.get("http_timeout_sec", 8.0)),
            bool(config.get("keep_alive", True)),
        )
    else:
        status, data, error = post_json(f"{gate_url}{login_path}", payload, float(config.get("http_timeout_sec", 8.0)))
    elapsed = (time.perf_counter() - started) * 1000.0
    if status == 200 and int(data.get("error", -1)) == 0:
        return OperationResult(True, elapsed, data=data)
    return OperationResult(False, elapsed, stage="gate_login", error=error or json.dumps(data, ensure_ascii=False), data=data)


def encode_frame(msg_id: int, payload: bytes) -> bytes:
    if len(payload) > 0xFFFF:
        raise ValueError("payload too large for MemoChat TCP frame")
    return struct.pack("!HH", msg_id, len(payload)) + payload


def recv_exact(sock: socket.socket, size: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < size:
        chunk = sock.recv(size - len(chunks))
        if not chunk:
            raise ConnectionError("socket closed before enough data was received")
        chunks.extend(chunk)
    return bytes(chunks)


def recv_frame(sock: socket.socket) -> tuple[int, bytes]:
    msg_id, size = struct.unpack("!HH", recv_exact(sock, 4))
    return msg_id, recv_exact(sock, size)


def tcp_chat_login(host: str, port: int, gate_response: dict[str, Any], timeout: float = 8.0) -> OperationResult:
    started = time.perf_counter()
    payload = {
        "uid": int(gate_response.get("uid", 0)),
        "token": gate_response.get("token", ""),
        "login_ticket": gate_response.get("login_ticket", ""),
        "ticket_expire_ms": gate_response.get("ticket_expire_ms", 0),
        "client_ver": "3.0.0",
        "protocol_version": 3,
    }
    try:
        with socket.create_connection((host, int(port)), timeout=timeout) as sock:
            sock.settimeout(timeout)
            sock.sendall(encode_frame(MSG_CHAT_LOGIN, json.dumps(payload, separators=(",", ":")).encode("utf-8")))
            msg_id, rsp_payload = recv_frame(sock)
        elapsed = (time.perf_counter() - started) * 1000.0
        if msg_id != MSG_CHAT_LOGIN_RSP:
            return OperationResult(False, elapsed, stage="chat_login", error=f"unexpected msg_id={msg_id}")
        rsp = json.loads(rsp_payload.decode("utf-8"))
        error_code = int(rsp.get("error", -1))
        return OperationResult(error_code == 0, elapsed, stage="chat_login", error_code=error_code, data=rsp)
    except Exception as exc:
        elapsed = (time.perf_counter() - started) * 1000.0
        return OperationResult(False, elapsed, stage="chat_login", error=str(exc))


def run_http_login(config: dict[str, Any], accounts: list[Account], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    picker = GateUrlPicker(gate_urls_from_config(config))
    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, lambda i: gate_login(config, accounts[i % len(accounts)], picker.next()))
    scenario = scenario_from_operations("http_login", results, started_at, wall_elapsed_sec)
    scenario.details["gate_urls"] = gate_urls_from_config(config)
    scenario.details["fast_http"] = bool(config.get("fast_http", False))
    scenario.details["keep_alive"] = bool(config.get("keep_alive", False))
    return scenario


def run_tcp_login(config: dict[str, Any], accounts: list[Account], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    picker = GateUrlPicker(gate_urls_from_config(config))

    def one(i: int) -> OperationResult:
        gate = gate_login(config, accounts[i % len(accounts)], picker.next())
        if not gate.ok:
            return gate
        data = gate.data
        host = data.get("host") or data.get("tcp_host") or "127.0.0.1"
        if host in {"0.0.0.0", "::", "::1"}:
            host = "127.0.0.1"
        port = int(data.get("port") or data.get("tcp_port") or 0)
        if port <= 0:
            return OperationResult(False, gate.elapsed_ms, stage="chat_endpoint", error="missing chat port")
        chat = tcp_chat_login(host, port, data, float(config.get("tcp_timeout_sec", 8.0)))
        chat.elapsed_ms += gate.elapsed_ms
        chat.data["gate_ms"] = gate.elapsed_ms
        return chat

    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, one)
    scenario = scenario_from_operations("tcp_login", results, started_at, wall_elapsed_sec)
    scenario.details["gate_urls"] = gate_urls_from_config(config)
    scenario.details["fast_http"] = bool(config.get("fast_http", False))
    scenario.details["keep_alive"] = bool(config.get("keep_alive", False))
    return scenario


def run_health(config: dict[str, Any], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    url = config.get("ai_base_url", "http://127.0.0.1:8096").rstrip("/") + "/health"

    def one(_: int) -> OperationResult:
        started = time.perf_counter()
        try:
            with urllib.request.urlopen(url, timeout=float(config.get("ai_timeout_sec", 10.0))) as resp:
                data = json.loads(resp.read().decode("utf-8"))
            elapsed = (time.perf_counter() - started) * 1000.0
            return OperationResult(resp.status == 200 and data.get("status") == "ok", elapsed, data=data)
        except Exception as exc:
            elapsed = (time.perf_counter() - started) * 1000.0
            return OperationResult(False, elapsed, stage="ai_health", error=str(exc))

    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, one)
    return scenario_from_operations("ai_health", results, started_at, wall_elapsed_sec)


def run_agent(config: dict[str, Any], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    url = config.get("ai_base_url", "http://127.0.0.1:8096").rstrip("/") + "/agent/run"
    prompts = config.get("agent_prompts") or ["请用一句话介绍 MemoChat。"]

    def one(i: int) -> OperationResult:
        started = time.perf_counter()
        payload = {
            "uid": int(config.get("ai_uid", 10001)),
            "session_id": f"loadtest-{uuid.uuid4().hex[:8]}",
            "content": prompts[i % len(prompts)],
            "deployment_preference": config.get("deployment_preference", "any"),
        }
        status, data, error = post_json(url, payload, float(config.get("ai_timeout_sec", 60.0)))
        elapsed = (time.perf_counter() - started) * 1000.0
        ok = status == 200 and int(data.get("code", -1)) == 0 and bool(data.get("content", ""))
        return OperationResult(ok, elapsed, stage="agent_run", error=error or data.get("message", ""), data=data)

    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, one)
    return scenario_from_operations("agent_run", results, started_at, wall_elapsed_sec)


def run_rag(config: dict[str, Any], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    base_url = config.get("ai_base_url", "http://127.0.0.1:8096").rstrip("/")
    uid = int(config.get("ai_uid", 10001))
    docs = config.get("rag_docs") or [
        {
            "file_name": "memochat-loadtest.md",
            "file_type": "md",
            "content": "MemoChat 的 GateServer 默认端口是 8080。ChatServer 使用 TCP 长连接承载聊天登录。",
            "expected_terms": ["GateServer", "8080"],
            "query": "MemoChat 的 GateServer 默认端口是什么？",
        }
    ]
    for doc in docs:
        content_b64 = base64.b64encode(doc["content"].encode("utf-8")).decode("ascii")
        post_json(
            f"{base_url}/kb/upload",
            {"uid": uid, "file_name": doc["file_name"], "file_type": doc.get("file_type", "md"), "content": content_b64},
            float(config.get("ai_timeout_sec", 60.0)),
        )

    def one(i: int) -> OperationResult:
        doc = docs[i % len(docs)]
        started = time.perf_counter()
        status, data, error = post_json(
            f"{base_url}/kb/search",
            {"uid": uid, "query": doc["query"], "top_k": int(config.get("rag_top_k", 3))},
            float(config.get("ai_timeout_sec", 30.0)),
        )
        elapsed = (time.perf_counter() - started) * 1000.0
        chunks = data.get("chunks", []) if isinstance(data, dict) else []
        combined = "\n".join(str(chunk.get("content", "")) for chunk in chunks)
        expected = doc.get("expected_terms", [])
        matched = sum(1 for term in expected if term in combined)
        recall = matched / len(expected) if expected else 1.0
        hallucination = 0.0 if recall >= 1.0 else 1.0
        ok = status == 200 and int(data.get("code", -1)) == 0 and bool(chunks)
        return OperationResult(
            ok,
            elapsed,
            stage="rag_search",
            error=error,
            data={"recall": recall, "hallucination": hallucination, "chunks": len(chunks)},
        )

    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, one)
    scenario = scenario_from_operations("rag_search", results, started_at, wall_elapsed_sec)
    recalls = [float(r.data.get("recall", 0.0)) for r in results]
    hallucinations = [float(r.data.get("hallucination", 1.0)) for r in results]
    scenario.quality["recall_at_k"] = round(sum(recalls) / len(recalls), 4) if recalls else 0.0
    scenario.quality["hallucination_rate"] = round(sum(hallucinations) / len(hallucinations), 4) if hallucinations else 0.0
    return scenario


def run_parallel(total: int, concurrency: int, fn) -> list[OperationResult]:
    total = max(0, int(total))
    concurrency = max(1, int(concurrency))
    results: list[OperationResult] = []
    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        futures = [executor.submit(fn, i) for i in range(total)]
        for future in as_completed(futures):
            results.append(future.result())
    return results


def run_timed_parallel(total: int, concurrency: int, fn) -> tuple[list[OperationResult], float]:
    started = time.perf_counter()
    results = run_parallel(total, concurrency, fn)
    return results, time.perf_counter() - started


def scenario_from_operations(
    scenario: str,
    results: list[OperationResult],
    started_at: str,
    wall_elapsed_sec: float = 0.0,
) -> ScenarioResult:
    errors: dict[str, int] = {}
    for result in results:
        if not result.ok:
            key = result.stage if not result.error else f"{result.stage}:{result.error[:120]}"
            errors[key] = errors.get(key, 0) + 1
    success = sum(1 for result in results if result.ok)
    return ScenarioResult(
        scenario=scenario,
        total=len(results),
        success=success,
        failed=len(results) - success,
        latencies_ms=[result.elapsed_ms for result in results],
        started_at=started_at,
        finished_at=now_iso(),
        wall_elapsed_sec=round(max(0.0, wall_elapsed_sec), 6),
        errors=errors,
    )


def summarize_latencies(values: list[float]) -> dict[str, float]:
    if not values:
        return {
            "min_ms": 0.0,
            "avg_ms": 0.0,
            "p50_ms": 0.0,
            "p75_ms": 0.0,
            "p90_ms": 0.0,
            "p95_ms": 0.0,
            "p99_ms": 0.0,
            "max_ms": 0.0,
        }
    ordered = sorted(float(v) for v in values)

    def pct(p: float) -> float:
        index = max(0, min(len(ordered) - 1, math.ceil((p / 100.0) * len(ordered)) - 1))
        return round(ordered[index], 3)

    return {
        "min_ms": round(ordered[0], 3),
        "avg_ms": round(statistics.fmean(ordered), 3),
        "p50_ms": pct(50),
        "p75_ms": pct(75),
        "p90_ms": pct(90),
        "p95_ms": pct(95),
        "p99_ms": pct(99),
        "max_ms": round(ordered[-1], 3),
    }


def build_report(result: ScenarioResult) -> dict[str, Any]:
    avg_request_sec = max(0.001, sum(result.latencies_ms) / 1000.0 / max(1, result.total))
    wall_elapsed_sec = result.wall_elapsed_sec
    if wall_elapsed_sec <= 0:
        wall_elapsed_sec = (max(result.latencies_ms) if result.latencies_ms else 0.0) / 1000.0
    wall_elapsed_sec = max(0.001, wall_elapsed_sec)
    return {
        "scenario": result.scenario,
        "started_at": result.started_at,
        "finished_at": result.finished_at,
        "summary": {
            "total": result.total,
            "success": result.success,
            "failed": result.failed,
            "success_rate": round(result.success / result.total, 4) if result.total else 0.0,
            "avg_request_sec": round(avg_request_sec, 4),
            "wall_elapsed_sec": round(wall_elapsed_sec, 4),
            "throughput_rps": round(result.total / wall_elapsed_sec, 3) if result.total else 0.0,
            "latency_ms": summarize_latencies(result.latencies_ms),
        },
        "errors": result.errors,
        "quality": result.quality,
        "details": result.details,
    }


def write_report(report: dict[str, Any], report_dir: Path) -> Path:
    report_dir.mkdir(parents=True, exist_ok=True)
    path = report_dir / f"{report['scenario']}_{time.strftime('%Y%m%d_%H%M%S', time.gmtime())}.json"
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat Python-only load test runner")
    parser.add_argument("--config", default="config.json")
    parser.add_argument("--scenario", choices=["http", "tcp", "ai-health", "agent", "rag", "all"], default="http")
    parser.add_argument("--total", type=int, default=20)
    parser.add_argument("--concurrency", type=int, default=5)
    parser.add_argument("--accounts-csv", default="")
    parser.add_argument("--report-dir", default="")
    parser.add_argument("--prefer-last-password", action="store_true")
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    config = load_json(config_path) if config_path.exists() else {}
    config_dir = config_path.parent
    report_dir = Path(args.report_dir) if args.report_dir else resolve_path(config.get("report_dir", "reports"), config_dir)
    accounts_path_text = args.accounts_csv or config.get("accounts_csv", "")
    accounts: list[Account] = []
    if accounts_path_text:
        accounts = load_accounts(resolve_path(accounts_path_text, config_dir), args.prefer_last_password)

    scenarios = [args.scenario] if args.scenario != "all" else ["http", "tcp", "ai-health", "agent", "rag"]
    exit_code = 0
    for scenario in scenarios:
        if scenario in {"http", "tcp"} and not accounts:
            print(f"[ERROR] scenario {scenario} requires accounts_csv")
            exit_code = 1
            continue
        if scenario == "http":
            result = run_http_login(config, accounts, args.total, args.concurrency)
        elif scenario == "tcp":
            result = run_tcp_login(config, accounts, args.total, args.concurrency)
        elif scenario == "ai-health":
            result = run_health(config, args.total, args.concurrency)
        elif scenario == "agent":
            result = run_agent(config, args.total, args.concurrency)
        elif scenario == "rag":
            result = run_rag(config, args.total, args.concurrency)
        else:
            raise ValueError(f"unsupported scenario: {scenario}")
        report = build_report(result)
        path = write_report(report, report_dir)
        print(json.dumps(report["summary"], ensure_ascii=False))
        print(f"Report: {path}")
        if result.failed:
            exit_code = 1
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
