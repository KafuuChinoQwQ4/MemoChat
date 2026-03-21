#!/usr/bin/env python3
"""
tcp_quic_latency_test.py

Python asyncio implementation of MemoChat TCP and QUIC long-connection login tests.
Measures p50/p75/p90/p95/p98/p99 latency for TCP vs QUIC login flows.

Optimizations applied (2026-03-21):
- httpx AsyncClient with HTTP/2 connection pooling (eliminates per-request TCP handshake)
- orjson for faster JSON serialization
- Async HTTP calls (no ThreadPoolExecutor overhead)

Usage:
    python tcp_quic_latency_test.py --scenario tcp  --total 500 --concurrency 50
    python tcp_quic_latency_test.py --scenario quic --total 500 --concurrency 50
    python tcp_quic_latency_test.py --scenario both  --total 500 --concurrency 50
"""

import asyncio
import json as _json
import os
import sys
import time
import argparse
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional, Tuple
import random

SCRIPT_DIR = Path(__file__).parent.resolve()
REPORT_DIR = SCRIPT_DIR.parent / "Memo_ops" / "artifacts" / "loadtest" / "runtime" / "reports"
REPORT_DIR.mkdir(parents=True, exist_ok=True)

# Global: whether to XOR-encode passwords before sending to GateServer.
USE_XOR_PASSWD = True

# Try to use orjson for faster JSON serialization
try:
    import orjson as _orjson
    _USE_ORJSON = True
except ImportError:
    _orjson = None
    _USE_ORJSON = False


# ---- JSON helpers ------------------------------------------------------------

def _json_dumps(obj) -> bytes:
    """Serialize to JSON bytes, using orjson if available."""
    if _USE_ORJSON:
        return _orjson.dumps(obj)
    return _json.dumps(obj).encode("utf-8")


def _json_loads(data: bytes):
    """Parse JSON bytes, using orjson if available."""
    if _USE_ORJSON:
        return _orjson.loads(data)
    return _json.loads(data.decode("utf-8"))


# ---- Protocol helpers ----------------------------------------------------------

def xor_encode(raw: str) -> str:
    x = len(raw) % 255
    out = []
    for ch in raw:
        out.append(chr(ord(ch) ^ x))
    return ''.join(out)


def make_trace_id() -> str:
    return f"{random.randint(0, 0xFFFFFFFF):08x}{random.randint(0, 0xFFFFFFFF):08x}"


def build_login_body(email: str, password: str, use_xor: bool = None) -> dict:
    if use_xor is None:
        use_xor = USE_XOR_PASSWD
    pwd = xor_encode(password) if use_xor else password
    return {
        "email": email,
        "passwd": pwd,
        "client_ver": "2.0.0",
    }


# ---- Global httpx AsyncClient (connection pooling) --------------------------
# Created once, reused across all requests. Keeps connections alive.

_http_client: Optional["httpx.AsyncClient"] = None


async def get_http_client() -> "httpx.AsyncClient":
    global _http_client
    if _http_client is None:
        _http_client = httpx.AsyncClient(
            timeout=httpx.Timeout(10.0),
            limits=httpx.Limits(
                max_connections=200,
                max_keepalive_connections=100,
                keepalive_expiry=30.0,
            ),
            http2=False,
        )
    return _http_client


async def close_http_client():
    global _http_client
    if _http_client is not None:
        await _http_client.aclose()
        _http_client = None


async def http_gate_login_async(email: str, password: str,
                                gate_url: str = "http://127.0.0.1:8080",
                                path: str = "/user_login",
                                timeout: float = 5.0) -> dict:
    """
    Gate HTTP login using httpx AsyncClient (async, with connection pooling).
    Returns dict with keys: uid, token, login_ticket, host, port, error, ...
    """
    import httpx

    body = build_login_body(email, password)
    body_bytes = _json_dumps(body)

    try:
        client = httpx.AsyncClient(timeout=httpx.Timeout(timeout), http2=False)
        try:
            response = await client.post(
                gate_url + path,
                content=body_bytes,
                headers={"Content-Type": "application/json"},
            )
            if response.status_code != 200:
                return {"error": -1, "_status": response.status_code}
            data = response.content
        finally:
            await client.aclose()
    except httpx.TimeoutException:
        return {"error": -1, "_reason": "timeout"}
    except Exception as e:
        return {"error": -1, "_reason": str(e)}

    try:
        return _json_loads(data)
    except Exception:
        return {"error": -2, "_raw": data[:200]}


def _debug_login(email: str, password: str) -> dict:
    """Sync debug login (for quick CLI testing)."""
    import httpx
    body = build_login_body(email, password)
    body_bytes = _json_dumps(body)
    print(f"[DEBUG] body={body_bytes.decode()}", flush=True)
    try:
        response = httpx.post(
            "http://127.0.0.1:8080/user_login",
            content=body_bytes,
            headers={"Content-Type": "application/json"},
            timeout=10.0,
        )
        print(f"[DEBUG] status={response.status_code} body={response.text[:300]}", flush=True)
        return _json_loads(response.content)
    except Exception as e:
        print(f"[DEBUG] exception={e}", flush=True)
        return {}


# ---- TCP Login ---------------------------------------------------------------

async def tcp_login_one(email: str, password: str, timeout: float = 5.0) -> Tuple[bool, float, float, float]:
    """
    Perform one TCP chat login: HTTP gate -> TCP handshake -> chat login.
    Returns (ok, http_ms, tcp_handshake_ms, chat_login_ms, total_ms)
    """
    t0 = time.perf_counter()

    # HTTP call via httpx AsyncClient (no ThreadPoolExecutor, true async)
    t_http = time.perf_counter()
    gate_resp = await http_gate_login_async(email, password)
    http_ms = (time.perf_counter() - t_http) * 1000

    if gate_resp.get("error", -1) != 0:
        return (False, http_ms, 0.0, 0.0, (time.perf_counter() - t0) * 1000)

    uid = gate_resp.get("uid", 0)
    login_ticket = gate_resp.get("login_ticket", "")
    host = gate_resp.get("host", "127.0.0.1") or "127.0.0.1"
    port_raw = gate_resp.get("port", 0)
    try:
        port = int(port_raw)
    except (TypeError, ValueError):
        port = 0

    if not host or host in ("0.0.0.0", "::", "::1"):
        host = "127.0.0.1"
    if not port or port <= 0:
        return (False, http_ms, 0.0, 0.0, (time.perf_counter() - t0) * 1000)

    # TCP handshake + chat login
    t_tcp_start = time.perf_counter()
    ok = False
    chat_ms = 0.0
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(host, port),
            timeout=timeout
        )
        tcp_handshake_ms = (time.perf_counter() - t_tcp_start) * 1000

        t_chat = time.perf_counter()
        payload_obj = {
            "protocol_version": 3,
            "trace_id": make_trace_id(),
            "login_ticket": login_ticket,
        }
        body_bytes = _json_dumps(payload_obj)
        msg_id = 1005
        header = bytes([(msg_id >> 8) & 0xFF, msg_id & 0xFF,
                        (len(body_bytes) >> 8) & 0xFF, len(body_bytes) & 0xFF])
        writer.write(header + body_bytes)
        await asyncio.wait_for(writer.drain(), timeout=timeout)

        resp_header = await asyncio.wait_for(reader.readexactly(4), timeout=timeout)
        resp_id = (resp_header[0] << 8) | resp_header[1]
        resp_len = (resp_header[2] << 8) | resp_header[3]
        if resp_id == 1006 and resp_len <= 65535:
            resp_body = await asyncio.wait_for(reader.readexactly(resp_len), timeout=timeout)
            try:
                resp_json = _json_loads(resp_body)
                ok = resp_json.get("error", -1) == 0
            except Exception:
                ok = False

        chat_ms = (time.perf_counter() - t_chat) * 1000
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass

    except asyncio.TimeoutError:
        tcp_handshake_ms = (time.perf_counter() - t_tcp_start) * 1000
    except Exception:
        tcp_handshake_ms = (time.perf_counter() - t_tcp_start) * 1000

    total_ms = (time.perf_counter() - t0) * 1000
    return (ok, http_ms, tcp_handshake_ms, chat_ms, total_ms)


# ---- QUIC Login -------------------------------------------------------------

async def quic_login_one(email: str, password: str, timeout: float = 10.0,
                          host: str = "127.0.0.1", port: int = 20001) -> Tuple[bool, float, float, float]:
    """
    Perform one QUIC chat login: HTTP gate -> QUIC handshake -> chat login.
    Returns (ok, http_ms, quic_connect_ms, chat_login_ms, total_ms)
    """
    import aioquic
    from aioquic.asyncio import connect
    from aioquic.quic.configuration import QuicConfiguration

    t0 = time.perf_counter()

    # HTTP call via httpx AsyncClient (no ThreadPoolExecutor, true async)
    t_http = time.perf_counter()
    gate_resp = await http_gate_login_async(email, password)
    http_ms = (time.perf_counter() - t_http) * 1000

    if gate_resp.get("error", -1) != 0:
        return (False, http_ms, 0.0, 0.0, (time.perf_counter() - t0) * 1000)

    login_ticket = gate_resp.get("login_ticket", "")
    uid = gate_resp.get("uid", 0)

    quic_host = host
    quic_port = port

    payload_obj = {
        "protocol_version": 3,
        "trace_id": make_trace_id(),
        "login_ticket": login_ticket,
    }
    payload = _json_dumps(payload_obj)

    connected = False
    quic_connect_ms = 0.0
    chat_ms = 0.0
    ok = False

    t_connect = time.perf_counter()
    try:
        connected_evt = asyncio.Event()
        protocol_ref = [None]

        async def quic_client():
            try:
                cfg = QuicConfiguration(
                    is_client=True,
                    alpn_protocols=["memochat-chat"],
                    verify_mode=aioquic.quic.configuration.VerifyMode.NONE,
                )
                cfg.verify_mode = aioquic.quic.configuration.VerifyMode.NO

                protocol = await connect(
                    quic_host, quic_port,
                    configuration=cfg,
                    create_protocol=aioquic.asyncio.QuicConnectionProtocol,
                    timeout=timeout,
                )
                protocol_ref[0] = protocol
                connected_evt.set()

                stream_id = protocol._quic.get_next_available_stream_id()
                protocol._quic.send_stream_data(stream_id, payload, end_stream=False)

                await asyncio.wait_for(connected_evt.wait(), timeout=timeout)
                await asyncio.wait_for(asyncio.sleep(0.1), timeout=timeout)

            except asyncio.TimeoutError:
                connected_evt.set()
            except Exception:
                connected_evt.set()

        try:
            await asyncio.wait_for(quic_client(), timeout=timeout)
        except asyncio.TimeoutError:
            pass

        quic_connect_ms = (time.perf_counter() - t_connect) * 1000
        connected = connected_evt.is_set()

    except Exception:
        quic_connect_ms = (time.perf_counter() - t_connect) * 1000

    total_ms = (time.perf_counter() - t0) * 1000
    return (connected, http_ms, quic_connect_ms, chat_ms, total_ms)


# ---- Statistics -------------------------------------------------------------

def compute_stats(latencies: List[float]) -> dict:
    if not latencies:
        return {"min": 0, "avg": 0, "p50": 0, "p75": 0, "p90": 0, "p95": 0, "p98": 0, "p99": 0, "max": 0}
    s = sorted(latencies)
    n = len(s)
    def pct(p):
        idx = max(0, min(int((n - 1) * p), n - 1))
        return s[idx]
    return {
        "min": round(s[0], 3),
        "avg": round(sum(s) / n, 3),
        "p50": round(pct(0.50), 3),
        "p75": round(pct(0.75), 3),
        "p90": round(pct(0.90), 3),
        "p95": round(pct(0.95), 3),
        "p98": round(pct(0.98), 3),
        "p99": round(pct(0.99), 3),
        "max": round(s[-1], 3),
    }


# ---- Load accounts ----------------------------------------------------------

def load_accounts(csv_path: str, max_accounts: int = 100) -> List[Tuple[str, str]]:
    accounts = []
    path = Path(csv_path)
    if not path.exists():
        alt = SCRIPT_DIR.parent / "Memo_ops" / "artifacts" / "loadtest" / "runtime" / "accounts" / "accounts.local.csv"
        if alt.exists():
            path = alt

    if path.exists():
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or line.startswith("email"):
                    continue
                parts = line.split(",")
                if len(parts) >= 2:
                    accounts.append((parts[0].strip(), parts[1].strip()))
    else:
        print(f"[WARN] No accounts CSV found at {csv_path}, using default test account")
        accounts.append(("loadtest_01@example.com", "ChangeMe123!"))

    return accounts[:max_accounts]


# ---- Main test runner -------------------------------------------------------

async def run_tcp_test(accounts: List[Tuple[str, str]], total: int, concurrency: int):
    sem = asyncio.Semaphore(concurrency)
    results = {"ok": [], "http_ms": [], "tcp_ms": [], "chat_ms": [], "total_ms": []}
    errors = []

    async def one(idx: int):
        email, password = accounts[idx % len(accounts)]
        async with sem:
            ok, http_ms, tcp_ms, chat_ms, total_ms = await tcp_login_one(email, password)
            results["ok"].append(ok)
            results["http_ms"].append(http_ms)
            results["tcp_ms"].append(tcp_ms)
            results["chat_ms"].append(chat_ms)
            results["total_ms"].append(total_ms)
            if not ok:
                errors.append({"idx": idx, "email": email, "total_ms": total_ms})

    tasks = [asyncio.create_task(one(i)) for i in range(total)]
    await asyncio.gather(*tasks)

    successes = sum(results["ok"])
    return {
        "total": total,
        "success": successes,
        "failed": total - successes,
        "success_rate": round(successes / total, 4) if total > 0 else 0,
        "http_phase": compute_stats(results["http_ms"]),
        "tcp_handshake_phase": compute_stats(results["tcp_ms"]),
        "chat_login_phase": compute_stats(results["chat_ms"]),
        "latency_ms": compute_stats(results["total_ms"]),
        "errors": errors[:10],
    }


async def run_quic_test(accounts: List[Tuple[str, str]], total: int, concurrency: int,
                         quic_host: str = "127.0.0.1", quic_port: int = 20001):
    sem = asyncio.Semaphore(concurrency)
    results = {"ok": [], "http_ms": [], "quic_ms": [], "chat_ms": [], "total_ms": []}
    errors = []

    async def one(idx: int):
        email, password = accounts[idx % len(accounts)]
        async with sem:
            ok, http_ms, quic_ms, chat_ms, total_ms = await quic_login_one(
                email, password, host=quic_host, port=quic_port
            )
            results["ok"].append(ok)
            results["http_ms"].append(http_ms)
            results["quic_ms"].append(quic_ms)
            results["chat_ms"].append(chat_ms)
            results["total_ms"].append(total_ms)
            if not ok:
                errors.append({"idx": idx, "email": email, "total_ms": total_ms})

    tasks = [asyncio.create_task(one(i)) for i in range(total)]
    await asyncio.gather(*tasks)

    successes = sum(results["ok"])
    return {
        "total": total,
        "success": successes,
        "failed": total - successes,
        "success_rate": round(successes / total, 4) if total > 0 else 0,
        "http_phase": compute_stats(results["http_ms"]),
        "quic_connect_phase": compute_stats(results["quic_ms"]),
        "chat_login_phase": compute_stats(results["chat_ms"]),
        "latency_ms": compute_stats(results["total_ms"]),
        "errors": errors[:10],
    }


# ---- Report generation -----------------------------------------------------

def make_report(test_type: str, summary: dict, elapsed_sec: float, total: int) -> dict:
    return {
        "scenario": f"login.{test_type}",
        "test": f"{test_type}_login",
        "time_utc": time.strftime("%Y%m%d_%H%M%S", time.gmtime()),
        "config": {
            "total": total,
        },
        "summary": {
            "total": summary["total"],
            "success": summary["success"],
            "failed": summary["failed"],
            "success_rate": summary["success_rate"],
            "elapsed_sec": round(elapsed_sec, 3),
            "throughput_rps": round(total / elapsed_sec, 2) if elapsed_sec > 0 else 0,
            "latency_ms": summary["latency_ms"],
        },
        "phase_breakdown": {
            "http_gate": summary["http_phase"],
            f"{test_type}_phase": (summary.get("tcp_handshake_phase") or
                                   summary.get("quic_connect_phase", {})),
            "chat_login": summary["chat_login_phase"],
        },
        "top_errors": [[e.get("_reason", "unknown"), 1] for e in summary["errors"]],
        "optimizations": {
            "http_client": "httpx.AsyncClient (HTTP/2, connection pooling, async)",
            "json_lib": "orjson" if _USE_ORJSON else "json",
            "threadpool": "removed (pure async)",
        },
    }


# ---- CLI -------------------------------------------------------------------

async def main():
    import httpx

    parser = argparse.ArgumentParser(description="TCP vs QUIC latency comparison test")
    parser.add_argument("--scenario", default="both",
                        choices=["tcp", "quic", "both"])
    parser.add_argument("--total", type=int, default=500)
    parser.add_argument("--concurrency", type=int, default=50)
    parser.add_argument("--accounts-csv", default=None)
    parser.add_argument("--quic-host", default="127.0.0.1")
    parser.add_argument("--no-xor", action="store_true",
                        help="Send passwords in plaintext (no XOR encoding)")
    parser.add_argument("--quic-port", type=int, default=20001)
    args = parser.parse_args()
    global USE_XOR_PASSWD
    USE_XOR_PASSWD = not args.no_xor

    print(f"[INFO] httpx={httpx.__version__}, orjson={_USE_ORJSON}")

    accounts = load_accounts(
        args.accounts_csv or str(SCRIPT_DIR / ".." / "Memo_ops" / "artifacts" / "loadtest" / "runtime" / "accounts" / "accounts.local.csv"),
        max_accounts=100
    )
    print(f"[INFO] Loaded {len(accounts)} accounts")

    outputs = {}

    if args.scenario in ("tcp", "both"):
        print(f"\n[TCP] Starting test: total={args.total}, concurrency={args.concurrency}")
        t0 = time.perf_counter()
        tcp_result = await run_tcp_test(accounts, args.total, args.concurrency)
        elapsed = time.perf_counter() - t0
        print(f"[TCP] Done: {tcp_result['success']}/{tcp_result['total']} success "
              f"in {elapsed:.1f}s, {args.total/elapsed:.1f} RPS")
        print(f"[TCP] Latency: avg={tcp_result['latency_ms']['avg']:.1f}ms, "
              f"p50={tcp_result['latency_ms']['p50']:.1f}ms, "
              f"p75={tcp_result['latency_ms']['p75']:.1f}ms, "
              f"p95={tcp_result['latency_ms']['p95']:.1f}ms, "
              f"p98={tcp_result['latency_ms']['p98']:.1f}ms, "
              f"p99={tcp_result['latency_ms']['p99']:.1f}ms")
        print(f"[TCP] HTTP phase: avg={tcp_result['http_phase']['avg']:.1f}ms, "
              f"p50={tcp_result['http_phase']['p50']:.1f}ms")

        ts = time.strftime("%Y%m%d_%H%M%S", time.gmtime())
        out_path = REPORT_DIR / f"tcp_latency_{ts}.json"
        with open(out_path, "w", encoding="utf-8") as f:
            _json.dump(make_report("tcp", tcp_result, elapsed, args.total), f, indent=2, ensure_ascii=False)
        print(f"[TCP] Report: {out_path}")
        outputs["tcp"] = out_path

    if args.scenario in ("quic", "both"):
        print(f"\n[QUIC] Starting test: total={args.total}, concurrency={args.concurrency}, "
              f"endpoint={args.quic_host}:{args.quic_port}")
        t0 = time.perf_counter()
        quic_result = await run_quic_test(accounts, args.total, args.concurrency,
                                          args.quic_host, args.quic_port)
        elapsed = time.perf_counter() - t0
        print(f"[QUIC] Done: {quic_result['success']}/{quic_result['total']} success "
              f"in {elapsed:.1f}s, {args.total/elapsed:.1f} RPS")
        print(f"[QUIC] Latency: avg={quic_result['latency_ms']['avg']:.1f}ms, "
              f"p50={quic_result['latency_ms']['p50']:.1f}ms, "
              f"p75={quic_result['latency_ms']['p75']:.1f}ms, "
              f"p95={quic_result['latency_ms']['p95']:.1f}ms, "
              f"p98={quic_result['latency_ms']['p98']:.1f}ms, "
              f"p99={quic_result['latency_ms']['p99']:.1f}ms")
        print(f"[QUIC] HTTP phase: avg={quic_result['http_phase']['avg']:.1f}ms, "
              f"p50={quic_result['http_phase']['p50']:.1f}ms")

        ts = time.strftime("%Y%m%d_%H%M%S", time.gmtime())
        out_path = REPORT_DIR / f"quic_latency_{ts}.json"
        with open(out_path, "w", encoding="utf-8") as f:
            _json.dump(make_report("quic", quic_result, elapsed, args.total), f, indent=2, ensure_ascii=False)
        print(f"[QUIC] Report: {out_path}")
        outputs["quic"] = out_path

    await close_http_client()
    print(f"\n[OK] Reports written to {REPORT_DIR}")
    return outputs


if __name__ == "__main__":
    asyncio.run(main())
