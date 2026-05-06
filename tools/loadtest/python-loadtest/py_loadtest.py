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
import subprocess
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
ID_HEART_BEAT_REQ = 1023
ID_HEARTBEAT_RSP = 1024
ID_GET_RELATION_BOOTSTRAP_REQ = 1092
ID_GET_RELATION_BOOTSTRAP_RSP = 1093


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
    report_override: dict[str, Any] = field(default_factory=dict)


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


def delete_json(url: str, timeout: float = 8.0) -> tuple[int, dict[str, Any], str]:
    req = urllib.request.Request(
        url,
        headers={"X-Trace-Id": uuid.uuid4().hex},
        method="DELETE",
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


def repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def config_dir_from_path(config_path: Path) -> Path:
    return config_path.parent


def resolve_report_dir(config: dict[str, Any], config_path: Path) -> Path:
    return resolve_path(str(config.get("report_dir", "reports")), config_dir_from_path(config_path))


def k6_wrapper_path() -> Path:
    return repo_root() / "tools" / "loadtest" / "k6" / "run-http-bench.ps1"


def run_k6_http_login(config: dict[str, Any], total: int, concurrency: int) -> dict[str, Any]:
    config_path_text = str(config.get("_config_path", "")).strip()
    if not config_path_text:
        raise RuntimeError("missing _config_path in loadtest config")
    config_path = Path(config_path_text)
    report_dir = resolve_report_dir(config, config_path)
    report_dir.mkdir(parents=True, exist_ok=True)
    timestamp = time.strftime("%Y%m%d_%H%M%S", time.gmtime())
    summary_path = report_dir / f"k6_http_login_{timestamp}.json"
    wrapper = k6_wrapper_path()
    if not wrapper.exists():
        raise RuntimeError(f"k6 wrapper not found: {wrapper}")

    cmd = [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(wrapper),
        "-ConfigPath",
        str(config_path),
        "-Total",
        str(total),
        "-Concurrency",
        str(concurrency),
        "-SummaryPath",
        str(summary_path),
    ]
    proc = subprocess.run(cmd, cwd=str(repo_root()), capture_output=True, text=True)
    if proc.returncode != 0 and not summary_path.exists():
        raise RuntimeError(
            "k6 benchmark failed:\n"
            f"cmd={' '.join(cmd)}\n"
            f"exit_code={proc.returncode}\n"
            f"stdout={proc.stdout}\n"
            f"stderr={proc.stderr}"
        )
    if not summary_path.exists():
        raise RuntimeError(f"k6 summary not found: {summary_path}")

    report = json.loads(summary_path.read_text(encoding="utf-8"))
    report.setdefault("details", {})
    report["details"].update(
        {
            "runner": "k6",
            "wrapper": str(wrapper),
            "summary_path": str(summary_path),
            "stdout": proc.stdout[-4000:],
            "stderr": proc.stderr[-4000:],
        }
    )
    report["details"]["exit_code"] = proc.returncode
    return report


def sweep_concurrency_points(config: dict[str, Any]) -> list[int]:
    raw = config.get("sweep_concurrency")
    if isinstance(raw, list) and raw:
        points = []
        for item in raw:
            try:
                value = int(item)
            except (TypeError, ValueError):
                continue
            if value > 0:
                points.append(value)
        if points:
            return sorted(set(points))
    return [1, 5, 10, 20, 50, 100, 200, 500]


def run_http_sweep(config: dict[str, Any], total: int) -> ScenarioResult:
    started_at = now_iso()
    runs: list[dict[str, Any]] = []
    points = sweep_concurrency_points(config)
    for concurrency in points:
        run_total = max(total, concurrency * int(config.get("sweep_iterations_per_vu", 10)))
        run_report = run_k6_http_login(config, run_total, concurrency)
        runs.append(run_report)

    best_rps = max((float(run["summary"].get("throughput_rps", 0.0)) for run in runs), default=0.0)
    worst_p99 = max((float(run["summary"]["latency_ms"].get("p99_ms", 0.0)) for run in runs), default=0.0)
    combined = {
        "scenario": "http_sweep",
        "started_at": started_at,
        "finished_at": now_iso(),
        "summary": {
            "total_runs": len(runs),
            "concurrency_points": points,
            "best_throughput_rps": round(best_rps, 3),
            "worst_p99_ms": round(worst_p99, 3),
        },
        "details": {"runs": runs},
    }
    return ScenarioResult(
        scenario="http_sweep",
        total=len(runs),
        success=len(runs),
        failed=0,
        latencies_ms=[],
        started_at=started_at,
        finished_at=combined["finished_at"],
        report_override=combined,
    )


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


def run_http_login_k6(config: dict[str, Any], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    report = run_k6_http_login(config, total, concurrency)
    summary = report.get("summary", {})
    return ScenarioResult(
        scenario="http_login_k6",
        total=int(summary.get("total", total)),
        success=int(summary.get("success", 0)),
        failed=int(summary.get("failed", 0)),
        latencies_ms=[],
        started_at=started_at,
        finished_at=report.get("finished_at", now_iso()),
        details={"runner": "k6", "report_path": report.get("details", {}).get("summary_path", "")},
        report_override=report,
    )


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


def open_logged_in_chat_socket(config: dict[str, Any], account: Account, gate_url: str) -> tuple[socket.socket, dict[str, Any]]:
    gate = gate_login(config, account, gate_url)
    if not gate.ok:
        raise RuntimeError(f"gate_login failed: {gate.error or gate.data}")
    data = gate.data
    host = data.get("host") or data.get("tcp_host") or "127.0.0.1"
    if host in {"0.0.0.0", "::", "::1"}:
        host = "127.0.0.1"
    port = int(data.get("port") or data.get("tcp_port") or 0)
    if port <= 0:
        raise RuntimeError("missing chat port")

    timeout = float(config.get("tcp_timeout_sec", 8.0))
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.settimeout(timeout)
    payload = {
        "uid": int(data.get("uid", 0)),
        "token": data.get("token", ""),
        "login_ticket": data.get("login_ticket", ""),
        "ticket_expire_ms": data.get("ticket_expire_ms", 0),
        "client_ver": str(config.get("client_ver", "3.0.0")),
        "protocol_version": 3,
        "trace_id": uuid.uuid4().hex,
    }
    try:
        sock.sendall(encode_frame(MSG_CHAT_LOGIN, json.dumps(payload, separators=(",", ":")).encode("utf-8")))
        msg_id, rsp_payload = recv_expected_frame(sock, {MSG_CHAT_LOGIN_RSP}, timeout)
        if msg_id != MSG_CHAT_LOGIN_RSP:
            raise RuntimeError(f"unexpected login msg_id={msg_id}")
        rsp = json.loads(rsp_payload.decode("utf-8"))
        error_code = int(rsp.get("error", -1))
        if error_code != 0:
            raise RuntimeError(f"chat_login error={error_code}")
        return sock, {"gate": data, "chat_login": rsp, "host": host, "port": port}
    except Exception:
        sock.close()
        raise


def recv_expected_frame(sock: socket.socket, expected_ids: set[int], timeout: float) -> tuple[int, bytes]:
    deadline = time.perf_counter() + timeout
    last_msg_id = 0
    while time.perf_counter() < deadline:
        sock.settimeout(max(0.05, deadline - time.perf_counter()))
        msg_id, payload = recv_frame(sock)
        if msg_id in expected_ids:
            return msg_id, payload
        last_msg_id = msg_id
    raise TimeoutError(f"expected frame {sorted(expected_ids)}, last_msg_id={last_msg_id}")


def tcp_request_on_socket(
    sock: socket.socket,
    msg_id: int,
    expected_msg_id: int,
    payload: dict[str, Any],
    timeout: float,
    stage: str,
) -> OperationResult:
    started = time.perf_counter()
    try:
        payload.setdefault("trace_id", uuid.uuid4().hex)
        payload.setdefault("protocol_version", 3)
        sock.sendall(encode_frame(msg_id, json.dumps(payload, separators=(",", ":")).encode("utf-8")))
        rsp_msg_id, rsp_payload = recv_expected_frame(sock, {expected_msg_id}, timeout)
        elapsed = (time.perf_counter() - started) * 1000.0
        if rsp_msg_id != expected_msg_id:
            return OperationResult(False, elapsed, stage=stage, error=f"unexpected msg_id={rsp_msg_id}")
        rsp = json.loads(rsp_payload.decode("utf-8")) if rsp_payload else {}
        error_code = int(rsp.get("error", 0))
        return OperationResult(error_code == 0, elapsed, stage=stage, error_code=error_code, data=rsp)
    except Exception as exc:
        elapsed = (time.perf_counter() - started) * 1000.0
        return OperationResult(False, elapsed, stage=stage, error=str(exc))


def run_http_login(config: dict[str, Any], accounts: list[Account], total: int, concurrency: int) -> ScenarioResult:
    if bool(config.get("use_k6_http", True)):
        return run_http_login_k6(config, total, concurrency)

    started_at = now_iso()
    picker = GateUrlPicker(gate_urls_from_config(config))
    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, lambda i: gate_login(config, accounts[i % len(accounts)], picker.next()))
    scenario = scenario_from_operations("http_login", results, started_at, wall_elapsed_sec)
    scenario.details["gate_urls"] = gate_urls_from_config(config)
    scenario.details["fast_http"] = bool(config.get("fast_http", False))
    scenario.details["keep_alive"] = bool(config.get("keep_alive", False))
    return scenario


def run_tcp_logged_in_requests(
    config: dict[str, Any],
    accounts: list[Account],
    total: int,
    concurrency: int,
    scenario_name: str,
    req_msg_id: int,
    rsp_msg_id: int,
    stage: str,
) -> ScenarioResult:
    setup_started_at = now_iso()
    session_count = max(1, min(int(concurrency), int(total)))
    picker = GateUrlPicker(gate_urls_from_config(config))
    setup_errors: dict[str, int] = {}
    setup_errors_lock = threading.Lock()

    def setup_one(i: int) -> tuple[socket.socket, dict[str, Any]] | None:
        try:
            return open_logged_in_chat_socket(config, accounts[i % len(accounts)], picker.next())
        except Exception as exc:
            key = f"setup:{str(exc)[:120]}"
            with setup_errors_lock:
                setup_errors[key] = setup_errors.get(key, 0) + 1
            return None

    setup_results = run_parallel(session_count, session_count, setup_one)
    sessions = [item for item in setup_results if item is not None]
    if not sessions:
        result = ScenarioResult(
            scenario=scenario_name,
            total=total,
            success=0,
            failed=total,
            latencies_ms=[],
            started_at=setup_started_at,
            finished_at=now_iso(),
            errors=setup_errors or {"setup:no logged-in sessions": total},
        )
        result.details["setup_sessions_requested"] = session_count
        result.details["setup_sessions_ready"] = 0
        return result

    per_worker = [total // len(sessions)] * len(sessions)
    for i in range(total % len(sessions)):
        per_worker[i] += 1
    timeout = float(config.get("tcp_timeout_sec", 8.0))
    started_at = now_iso()
    started = time.perf_counter()

    def run_worker(index: int) -> list[OperationResult]:
        sock, login_meta = sessions[index]
        results: list[OperationResult] = []
        uid = int(login_meta.get("gate", {}).get("uid", 0))
        for _ in range(per_worker[index]):
            payload = {"uid": uid} if uid > 0 else {}
            results.append(tcp_request_on_socket(sock, req_msg_id, rsp_msg_id, payload, timeout, stage))
        return results

    worker_results = run_parallel(len(sessions), len(sessions), run_worker)
    wall_elapsed_sec = time.perf_counter() - started
    results = [result for batch in worker_results for result in batch]
    for sock, _ in sessions:
        try:
            sock.close()
        except Exception:
            pass
    scenario = scenario_from_operations(scenario_name, results, started_at, wall_elapsed_sec)
    scenario.errors.update(setup_errors)
    scenario.details["setup_started_at"] = setup_started_at
    scenario.details["setup_sessions_requested"] = session_count
    scenario.details["setup_sessions_ready"] = len(sessions)
    scenario.details["measured_phase"] = "logged_in_tcp_requests_only"
    scenario.details["gate_urls"] = gate_urls_from_config(config)
    return scenario


def run_tcp_heartbeat(config: dict[str, Any], accounts: list[Account], total: int, concurrency: int) -> ScenarioResult:
    return run_tcp_logged_in_requests(
        config,
        accounts,
        total,
        concurrency,
        "tcp_heartbeat",
        ID_HEART_BEAT_REQ,
        ID_HEARTBEAT_RSP,
        "tcp_heartbeat",
    )


def run_tcp_relation_bootstrap(config: dict[str, Any], accounts: list[Account], total: int, concurrency: int) -> ScenarioResult:
    return run_tcp_logged_in_requests(
        config,
        accounts,
        total,
        concurrency,
        "tcp_relation_bootstrap",
        ID_GET_RELATION_BOOTSTRAP_REQ,
        ID_GET_RELATION_BOOTSTRAP_RSP,
        "tcp_relation_bootstrap",
    )


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
        chat.data["gate_stage_metrics"] = gate.data.get("stage_metrics", {})
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


def default_rag_docs() -> list[dict[str, Any]]:
    return [
        {
            "file_name": "memochat-architecture-loadtest.md",
            "file_type": "md",
            "content": "\n\n".join(
                [
                    "MemoChat architecture note. GateServer exposes the public HTTP entrypoint on port 8080 and forwards AI requests to AIServer. ChatServer keeps TCP long connections for chat login and realtime messages.",
                    "AIServer bridges the C++ services to AIOrchestrator. AIOrchestrator listens on port 8096 and coordinates LLM calls, tools, knowledge search, graph memory, and observability traces.",
                    "Qdrant is the vector database for knowledge-base chunks. The local Docker deployment publishes Qdrant HTTP on 6333 and gRPC on 6334, while Neo4j publishes Browser on 7474 and Bolt on 7687.",
                    "A distractor service called MemoArchive uses port 18080 in old notes. That value must not be confused with the GateServer public port for MemoChat.",
                ]
            ),
            "queries": [
                {
                    "query": "MemoChat 对外 HTTP 入口 GateServer 监听哪个端口？",
                    "expected_terms": ["GateServer", "8080"],
                },
                {
                    "query": "AIOrchestrator 默认服务端口是多少，负责哪些 AI 编排能力？",
                    "expected_terms": ["AIOrchestrator", "8096", "knowledge search"],
                },
                {
                    "query": "Qdrant 在本地 Docker 里发布了哪些端口？",
                    "expected_terms": ["Qdrant", "6333", "6334"],
                },
            ],
        },
        {
            "file_name": "memochat-loadtest-observability.md",
            "file_type": "md",
            "content": "\n\n".join(
                [
                    "MemoChat layered load tests separate app endpoints from dependency capacity. The Python runner covers TCP login, AI health, agent runs, and RAG retrieval. k6 is used for HTTP-only health and login baselines.",
                    "Runtime reports should compare health, HTTP login, TCP login, sweep curves, dependency checks, and deep AI paths separately. Mixing these layers into one headline QPS hides the bottleneck.",
                    "Observability containers keep stable ports: Prometheus 9090, Grafana 3000, Loki 3100, Tempo 3200, cAdvisor 8088, and InfluxDB 8086. AI traces can be correlated by trace_id.",
                    "A distractor benchmark named legacy-latency once reported port 9099 for an unrelated metrics daemon. Current MemoChat Prometheus remains on 9090.",
                ]
            ),
            "queries": [
                {
                    "query": "MemoChat 分层压测为什么不能只看一个总 QPS？",
                    "expected_terms": ["separate app endpoints", "dependency capacity", "bottleneck"],
                },
                {
                    "query": "Prometheus、Grafana、Loki、Tempo 的本地端口分别是什么？",
                    "expected_terms": ["Prometheus 9090", "Grafana 3000", "Loki 3100", "Tempo 3200"],
                },
                {
                    "query": "哪个工具用于 HTTP-only health 和 login baseline？",
                    "expected_terms": ["k6", "HTTP-only"],
                },
            ],
        },
        {
            "file_name": "memochat-agent-rag-quality.md",
            "file_type": "md",
            "content": "\n\n".join(
                [
                    "RAG quality should report recall at multiple cutoffs, such as recall@1, recall@3, recall@5, and recall@10. A tiny single-document test can produce 100 percent recall without proving ranking quality.",
                    "A stronger retrieval corpus contains multiple documents, repeated product terms, near-miss port numbers, paraphrased questions, and unrelated distractor paragraphs. The top5 set should be inspected because agent context usually receives several chunks.",
                    "For load tests, each uploaded knowledge base should be deleted after the scenario so old collections do not inflate recall. The scenario should include per-query misses for debugging.",
                    "A distractor sentence says top2 is enough for every system. MemoChat does not treat that as a quality target; top5 and top10 remain important for recall analysis.",
                ]
            ),
            "queries": [
                {
                    "query": "为什么单文档 RAG 测试出现 100% recall 不能说明排序质量？",
                    "expected_terms": ["single-document", "100 percent recall", "ranking quality"],
                },
                {
                    "query": "更暴力的检索测试语料应该包含什么干扰因素？",
                    "expected_terms": ["multiple documents", "near-miss port numbers", "paraphrased questions", "distractor"],
                },
                {
                    "query": "RAG 压测结束后为什么要删除上传的知识库？",
                    "expected_terms": ["deleted after the scenario", "old collections", "inflate recall"],
                },
            ],
        },
    ]


def synthetic_rag_distractor_docs(count: int) -> list[dict[str, Any]]:
    topics = [
        ("gateway-ports", "GateServer", "8080", "18080", "public HTTP entrypoint"),
        ("ai-routing", "AIOrchestrator", "8096", "8099", "agent orchestration"),
        ("vector-store", "Qdrant", "6333", "6633", "knowledge chunks"),
        ("metrics-stack", "Prometheus", "9090", "9099", "observability metrics"),
        ("loadtest-layers", "k6", "HTTP-only", "tcp-only", "baseline runner"),
        ("rag-quality", "recall@5", "top5", "top2", "retrieval quality"),
    ]
    docs: list[dict[str, Any]] = []
    for index in range(max(0, count)):
        topic, service, true_hint, near_miss, phrase = topics[index % len(topics)]
        docs.append(
            {
                "file_name": f"memochat-rag-distractor-{index + 1:02d}-{topic}.md",
                "file_type": "md",
                "content": "\n\n".join(
                    [
                        f"Distractor note {index + 1}. {service} appears in historical migration notes with overlapping MemoChat terminology, but this paragraph is not the authoritative answer for the active benchmark query.",
                        f"The note repeats {service}, MemoChat, load test, agent, RAG, vector search, and {phrase}. It also includes a near-miss value {near_miss} next to the real-looking token {true_hint} to test ranking under noisy context.",
                        f"Legacy runbooks sometimes mixed unrelated services with similar ports and labels. The benchmark should prefer the explicitly uploaded architecture, observability, or RAG quality documents instead of this distractor document.",
                        f"Extra filler for chunk pressure: MemoChat retrieval context, top-k ranking, AI agent evaluation, Qdrant collection cleanup, isolated uid, benchmark report, latency, throughput, recall, and trace_id are repeated here.",
                    ]
                ),
            }
        )
    return docs


def expand_rag_queries(docs: list[dict[str, Any]]) -> list[dict[str, Any]]:
    queries: list[dict[str, Any]] = []
    for doc_index, doc in enumerate(docs):
        raw_queries = doc.get("queries")
        if isinstance(raw_queries, list) and raw_queries:
            for query_index, raw_query in enumerate(raw_queries):
                if not isinstance(raw_query, dict):
                    continue
                query = str(raw_query.get("query", "")).strip()
                if not query:
                    continue
                queries.append(
                    {
                        "query_id": str(raw_query.get("query_id") or f"doc{doc_index + 1}-q{query_index + 1}"),
                        "query": query,
                        "expected_terms": [str(term) for term in raw_query.get("expected_terms", [])],
                        "expected_source": str(raw_query.get("expected_source") or doc.get("file_name", "")),
                    }
                )
        elif doc.get("query"):
            queries.append(
                {
                    "query_id": f"doc{doc_index + 1}-q1",
                    "query": str(doc["query"]),
                    "expected_terms": [str(term) for term in doc.get("expected_terms", [])],
                    "expected_source": str(doc.get("file_name", "")),
                }
            )
    return queries


def normalize_rag_cutoffs(config: dict[str, Any]) -> list[int]:
    raw_cutoffs = config.get("rag_eval_cutoffs", [1, 3, 5, 10])
    if not isinstance(raw_cutoffs, list):
        raw_cutoffs = [1, 3, 5, 10]
    cutoffs = sorted({max(1, int(value)) for value in raw_cutoffs})
    if not cutoffs:
        cutoffs = [1, 3, 5, 10]
    return cutoffs


def recall_for_chunks(chunks: list[dict[str, Any]], expected_terms: list[str], cutoff: int) -> float:
    if not expected_terms:
        return 1.0
    combined = "\n".join(str(chunk.get("content", "")) for chunk in chunks[:cutoff])
    matched = sum(1 for term in expected_terms if term and term in combined)
    return matched / len(expected_terms)


def run_rag(config: dict[str, Any], total: int, concurrency: int) -> ScenarioResult:
    started_at = now_iso()
    base_url = config.get("ai_base_url", "http://127.0.0.1:8096").rstrip("/")
    base_uid = int(config.get("ai_uid", 10001))
    isolate_uid = bool(config.get("rag_isolate_uid", True))
    uid = base_uid
    if isolate_uid:
        uid = 900_000_000 + int(uuid.uuid4().hex[:6], 16) % 90_000_000
    docs = list(config.get("rag_docs") or default_rag_docs())
    docs.extend(synthetic_rag_distractor_docs(int(config.get("rag_synthetic_distractor_docs", 12))))
    docs = [doc for doc in docs if isinstance(doc, dict) and str(doc.get("content", "")).strip()]
    queries = expand_rag_queries(docs)
    cutoffs = normalize_rag_cutoffs(config)
    requested_top_k = int(config.get("rag_top_k", 5))
    search_top_k = max(requested_top_k, max(cutoffs), 1)
    cleanup_uploads = bool(config.get("rag_cleanup_uploads", True))
    uploaded_kb_ids: list[str] = []
    upload_errors: list[str] = []
    for doc in docs:
        content_b64 = base64.b64encode(doc["content"].encode("utf-8")).decode("ascii")
        status, data, error = post_json(
            f"{base_url}/kb/upload",
            {"uid": uid, "file_name": doc["file_name"], "file_type": doc.get("file_type", "md"), "content": content_b64},
            float(config.get("ai_timeout_sec", 60.0)),
        )
        if status == 200 and isinstance(data, dict) and data.get("kb_id"):
            uploaded_kb_ids.append(str(data["kb_id"]))
        else:
            upload_errors.append(f"{doc.get('file_name', '<unnamed>')}: {error or data.get('detail') or data.get('message') or status}")

    if upload_errors and not bool(config.get("rag_allow_partial_uploads", False)):
        results = [
            OperationResult(
                False,
                0.0,
                stage="rag_upload",
                error="; ".join(upload_errors[:3]),
                data={"upload_errors": upload_errors},
            )
            for _ in range(max(1, total))
        ]
        scenario = scenario_from_operations("rag_search", results, started_at, 0.0)
        scenario.details["rag_doc_count"] = len(docs)
        scenario.details["rag_query_count"] = len(queries)
        scenario.details["rag_base_uid"] = base_uid
        scenario.details["rag_effective_uid"] = uid
        scenario.details["rag_upload_errors"] = upload_errors[:10]
        return scenario

    if not queries:
        queries = [
            {
                "query_id": "fallback-q1",
                "query": "MemoChat GateServer 端口",
                "expected_terms": ["GateServer"],
                "expected_source": "",
            }
        ]

    def one(i: int) -> OperationResult:
        query_case = queries[i % len(queries)]
        started = time.perf_counter()
        status, data, error = post_json(
            f"{base_url}/kb/search",
            {"uid": uid, "query": query_case["query"], "top_k": search_top_k},
            float(config.get("ai_timeout_sec", 30.0)),
        )
        elapsed = (time.perf_counter() - started) * 1000.0
        chunks = data.get("chunks", []) if isinstance(data, dict) else []
        expected = query_case.get("expected_terms", [])
        recall_by_k = {f"recall_at_{cutoff}": recall_for_chunks(chunks, expected, cutoff) for cutoff in cutoffs}
        recall = recall_by_k.get(f"recall_at_{requested_top_k}", recall_for_chunks(chunks, expected, requested_top_k))
        hallucination = 0.0 if recall >= 1.0 else 1.0
        ok = status == 200 and int(data.get("code", -1)) == 0 and bool(chunks)
        return OperationResult(
            ok,
            elapsed,
            stage="rag_search",
            error=error,
            data={
                "query_id": query_case.get("query_id", ""),
                "query": query_case["query"],
                "expected_terms": expected,
                "recall": recall,
                "recall_by_k": recall_by_k,
                "hallucination": hallucination,
                "chunks": len(chunks),
                "top_sources": [str(chunk.get("source", "")) for chunk in chunks[:search_top_k]],
            },
        )

    results, wall_elapsed_sec = run_timed_parallel(total, concurrency, one)
    scenario = scenario_from_operations("rag_search", results, started_at, wall_elapsed_sec)
    recalls = [float(r.data.get("recall", 0.0)) for r in results]
    hallucinations = [float(r.data.get("hallucination", 1.0)) for r in results]
    scenario.quality["recall_at_k"] = round(sum(recalls) / len(recalls), 4) if recalls else 0.0
    scenario.quality["recall_at_top_k"] = scenario.quality["recall_at_k"]
    for cutoff in cutoffs:
        key = f"recall_at_{cutoff}"
        values = [
            float(r.data.get("recall_by_k", {}).get(key, 0.0))
            for r in results
            if isinstance(r.data.get("recall_by_k", {}), dict)
        ]
        scenario.quality[key] = round(sum(values) / len(values), 4) if values else 0.0
    scenario.quality["hallucination_rate"] = round(sum(hallucinations) / len(hallucinations), 4) if hallucinations else 0.0
    scenario.details["rag_doc_count"] = len(docs)
    scenario.details["rag_query_count"] = len(queries)
    scenario.details["rag_base_uid"] = base_uid
    scenario.details["rag_effective_uid"] = uid
    scenario.details["rag_isolate_uid"] = isolate_uid
    scenario.details["rag_requested_top_k"] = requested_top_k
    scenario.details["rag_search_top_k"] = search_top_k
    scenario.details["rag_eval_cutoffs"] = cutoffs
    scenario.details["rag_uploaded_kbs"] = len(uploaded_kb_ids)
    if upload_errors:
        scenario.details["rag_upload_errors"] = upload_errors[:10]
    requested_key = f"recall_at_{requested_top_k}"
    search_key = f"recall_at_{search_top_k}"
    requested_misses = []
    deep_misses = []
    for result in results:
        recall_by_k = result.data.get("recall_by_k", {})
        if not isinstance(recall_by_k, dict):
            recall_by_k = {}
        miss = {
            "query_id": result.data.get("query_id", ""),
            "query": result.data.get("query", ""),
            "expected_terms": result.data.get("expected_terms", []),
            "recall_by_k": recall_by_k,
            "top_sources": result.data.get("top_sources", []),
        }
        requested_recall = float(recall_by_k.get(requested_key, result.data.get("recall", 0.0)))
        deep_recall = float(recall_by_k.get(search_key, result.data.get("recall", 0.0)))
        if requested_recall < 1.0:
            requested_misses.append(miss)
        if deep_recall < 1.0:
            deep_misses.append(miss)
    scenario.details["rag_misses"] = requested_misses[:20]
    scenario.details["rag_deep_misses"] = deep_misses[:20]
    if cleanup_uploads:
        cleanup_errors = []
        for kb_id in uploaded_kb_ids:
            status, data, error = delete_json(
                f"{base_url}/kb/{kb_id}?uid={uid}",
                float(config.get("ai_timeout_sec", 30.0)),
            )
            if status != 200 or int(data.get("code", -1)) != 0:
                cleanup_errors.append(f"{kb_id}: {error or data.get('detail') or data.get('message') or status}")
        scenario.details["rag_cleanup_attempted"] = len(uploaded_kb_ids)
        if cleanup_errors:
            scenario.details["rag_cleanup_errors"] = cleanup_errors[:10]
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
    stage_metrics: dict[str, list[float]] = {}
    for result in results:
        if not result.ok:
            key = result.stage if not result.error else f"{result.stage}:{result.error[:120]}"
            errors[key] = errors.get(key, 0) + 1
        if isinstance(result.data, dict):
            for source in (result.data.get("stage_metrics", {}), result.data.get("gate_stage_metrics", {})):
                if not isinstance(source, dict):
                    continue
                for metric_name, raw_value in source.items():
                    try:
                        stage_metrics.setdefault(str(metric_name), []).append(float(raw_value))
                    except (TypeError, ValueError):
                        continue
            if "gate_ms" in result.data:
                try:
                    stage_metrics.setdefault("gate_ms", []).append(float(result.data["gate_ms"]))
                except (TypeError, ValueError):
                    pass
    success = sum(1 for result in results if result.ok)
    scenario_result = ScenarioResult(
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
    if stage_metrics:
        scenario_result.details["stage_latency_ms"] = {
            name: summarize_latencies(values) for name, values in sorted(stage_metrics.items())
        }
    return scenario_result


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
    if result.report_override:
        return result.report_override
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
    parser.add_argument("--scenario", choices=["http", "tcp", "tcp-heartbeat", "tcp-relation-bootstrap", "sweep", "ai-health", "agent", "rag", "all"], default="http")
    parser.add_argument("--total", type=int, default=20)
    parser.add_argument("--concurrency", type=int, default=5)
    parser.add_argument("--accounts-csv", default="")
    parser.add_argument("--report-dir", default="")
    parser.add_argument("--prefer-last-password", action="store_true")
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    config = load_json(config_path) if config_path.exists() else {}
    config["_config_path"] = str(config_path)
    config["_config_dir"] = str(config_path.parent)
    config_dir = config_path.parent
    report_dir = Path(args.report_dir).resolve() if args.report_dir else resolve_path(config.get("report_dir", "reports"), config_dir)
    if args.report_dir:
        config["report_dir"] = str(report_dir)
    accounts_path_text = args.accounts_csv or config.get("accounts_csv", "")
    accounts: list[Account] = []
    if accounts_path_text:
        accounts = load_accounts(resolve_path(accounts_path_text, config_dir), args.prefer_last_password)

    scenarios = [args.scenario] if args.scenario != "all" else ["http", "tcp", "tcp-heartbeat", "tcp-relation-bootstrap", "sweep", "ai-health", "agent", "rag"]
    exit_code = 0
    for scenario in scenarios:
        if scenario in {"http", "tcp", "tcp-heartbeat", "tcp-relation-bootstrap"} and not accounts:
            print(f"[ERROR] scenario {scenario} requires accounts_csv")
            exit_code = 1
            continue
        if scenario == "http":
            result = run_http_login(config, accounts, args.total, args.concurrency)
        elif scenario == "tcp":
            result = run_tcp_login(config, accounts, args.total, args.concurrency)
        elif scenario == "tcp-heartbeat":
            result = run_tcp_heartbeat(config, accounts, args.total, args.concurrency)
        elif scenario == "tcp-relation-bootstrap":
            result = run_tcp_relation_bootstrap(config, accounts, args.total, args.concurrency)
        elif scenario == "sweep":
            result = run_http_sweep(config, args.total)
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
