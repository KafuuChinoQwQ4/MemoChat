#!/usr/bin/env python3
"""Storage and HTTP micro-benchmarks for MemoChat local resume reports."""

from __future__ import annotations

import argparse
import json
import math
import random
import socket
import string
import subprocess
import time
import urllib.request
import uuid
import queue
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

import psycopg
import pymongo
import redis


@dataclass
class BenchResult:
    name: str
    total: int
    success: int
    failed: int
    latencies_ms: list[float]
    started_at: str
    finished_at: str
    wall_elapsed_sec: float
    details: dict[str, Any]


def now_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = max(0, min(len(ordered) - 1, math.ceil((p / 100.0) * len(ordered)) - 1))
    return round(ordered[index], 3)


def summarize(result: BenchResult) -> dict[str, Any]:
    lat = result.latencies_ms
    wall = max(0.001, result.wall_elapsed_sec)
    return {
        "name": result.name,
        "started_at": result.started_at,
        "finished_at": result.finished_at,
        "summary": {
            "total": result.total,
            "success": result.success,
            "failed": result.failed,
            "success_rate": round(result.success / result.total, 4) if result.total else 0.0,
            "wall_elapsed_sec": round(wall, 4),
            "throughput_rps": round(result.total / wall, 3) if result.total else 0.0,
            "latency_ms": {
                "min_ms": round(min(lat), 3) if lat else 0.0,
                "avg_ms": round(sum(lat) / len(lat), 3) if lat else 0.0,
                "p50_ms": percentile(lat, 50),
                "p75_ms": percentile(lat, 75),
                "p90_ms": percentile(lat, 90),
                "p95_ms": percentile(lat, 95),
                "p99_ms": percentile(lat, 99),
                "max_ms": round(max(lat), 3) if lat else 0.0,
            },
        },
        "details": result.details,
    }


def run_parallel(name: str, total: int, concurrency: int, fn: Callable[[int], tuple[bool, dict[str, Any]]]) -> BenchResult:
    started_at = now_iso()
    started = time.perf_counter()
    latencies: list[float] = []
    failed = 0
    error_buckets: dict[str, int] = {}
    with ThreadPoolExecutor(max_workers=max(1, concurrency)) as executor:
        futures = {executor.submit(fn, i): i for i in range(total)}
        for future in as_completed(futures):
            elapsed_ms = 0.0
            try:
                ok, data = future.result()
                elapsed_ms = float(data.get("elapsed_ms", 0.0))
                if not ok:
                    failed += 1
                    key = str(data.get("error", "failed"))[:120]
                    error_buckets[key] = error_buckets.get(key, 0) + 1
            except Exception as exc:
                failed += 1
                key = str(exc)[:120]
                error_buckets[key] = error_buckets.get(key, 0) + 1
            latencies.append(elapsed_ms)
    wall = time.perf_counter() - started
    return BenchResult(name, total, total - failed, failed, latencies, started_at, now_iso(), wall, {"errors": error_buckets})


def bench_redis(total: int, concurrency: int) -> list[dict[str, Any]]:
    value = "x" * 256
    pool = redis.ConnectionPool(host="127.0.0.1", port=6379, password="123456", max_connections=max(8, concurrency * 2), decode_responses=False)

    def write_one(i: int) -> tuple[bool, dict[str, Any]]:
        key = f"bench:resume:{i}:{uuid.uuid4().hex}"
        started = time.perf_counter()
        try:
            client = redis.Redis(connection_pool=pool)
            client.set(key, value)
            return True, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    keys = [f"bench:resume:read:{i}" for i in range(max(1, min(total, 1000)))]
    client = redis.Redis(connection_pool=pool)
    for key in keys:
        client.set(key, value)

    def read_one(i: int) -> tuple[bool, dict[str, Any]]:
        started = time.perf_counter()
        try:
            client = redis.Redis(connection_pool=pool)
            raw = client.get(keys[i % len(keys)])
            return raw == value.encode(), {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    return [summarize(run_parallel("redis_set_256b", total, concurrency, write_one)),
            summarize(run_parallel("redis_get_256b", total, concurrency, read_one))]


def bench_postgres(total: int, concurrency: int) -> list[dict[str, Any]]:
    conninfo = "host=127.0.0.1 port=15432 dbname=memo_pg user=memochat password=123456"
    with psycopg.connect(conninfo, autocommit=True) as conn:
        conn.execute("CREATE SCHEMA IF NOT EXISTS bench;")
        conn.execute("CREATE TABLE IF NOT EXISTS bench.resume_kv (id text PRIMARY KEY, value text, updated_at timestamptz DEFAULT now());")
    payload = "p" * 256
    with psycopg.connect(conninfo, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.executemany(
                "INSERT INTO bench.resume_kv(id,value) VALUES (%s,%s) ON CONFLICT (id) DO UPDATE SET value=EXCLUDED.value;",
                [(f"seed_{i}", payload) for i in range(200)],
            )

    def write_one(i: int) -> tuple[bool, dict[str, Any]]:
        key = f"run_{uuid.uuid4().hex}"
        started = time.perf_counter()
        try:
            with psycopg.connect(conninfo, autocommit=True) as conn:
                conn.execute(
                    "INSERT INTO bench.resume_kv(id,value) VALUES (%s,%s) ON CONFLICT (id) DO UPDATE SET value=EXCLUDED.value, updated_at=now();",
                    (key, payload),
                )
            return True, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    def read_one(i: int) -> tuple[bool, dict[str, Any]]:
        started = time.perf_counter()
        try:
            with psycopg.connect(conninfo, autocommit=True) as conn:
                raw = conn.execute("SELECT length(value) FROM bench.resume_kv WHERE id=%s;", (f"seed_{i % 200}",)).fetchone()
            return raw and raw[0] == 256, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    reports = [
        summarize(run_parallel("postgres_upsert_256b_new_connection", total, concurrency, write_one)),
        summarize(run_parallel("postgres_pk_read_256b_new_connection", total, concurrency, read_one)),
    ]

    pool: queue.Queue[psycopg.Connection] = queue.Queue(maxsize=max(1, concurrency))
    for _ in range(max(1, concurrency)):
        pool.put(psycopg.connect(conninfo, autocommit=True))

    def pooled_write_one(i: int) -> tuple[bool, dict[str, Any]]:
        key = f"pool_{uuid.uuid4().hex}"
        started = time.perf_counter()
        conn = pool.get()
        try:
            conn.execute(
                "INSERT INTO bench.resume_kv(id,value) VALUES (%s,%s) ON CONFLICT (id) DO UPDATE SET value=EXCLUDED.value, updated_at=now();",
                (key, payload),
            )
            return True, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}
        finally:
            pool.put(conn)

    def pooled_read_one(i: int) -> tuple[bool, dict[str, Any]]:
        started = time.perf_counter()
        conn = pool.get()
        try:
            raw = conn.execute("SELECT length(value) FROM bench.resume_kv WHERE id=%s;", (f"seed_{i % 200}",)).fetchone()
            return raw and raw[0] == 256, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}
        finally:
            pool.put(conn)

    try:
        reports.extend([
            summarize(run_parallel("postgres_upsert_256b_pooled", total, concurrency, pooled_write_one)),
            summarize(run_parallel("postgres_pk_read_256b_pooled", total, concurrency, pooled_read_one)),
        ])
    finally:
        while not pool.empty():
            conn = pool.get_nowait()
            conn.close()
    return reports


def bench_mongo(total: int, concurrency: int) -> list[dict[str, Any]]:
    client = pymongo.MongoClient("mongodb://memochat_app:123456@127.0.0.1:27017/memochat", maxPoolSize=max(8, concurrency * 2))
    collection = client["memochat"]["resume_bench"]
    collection.create_index("_id")
    payload = "m" * 256
    for i in range(200):
        collection.update_one({"_id": f"seed_{i}"}, {"$set": {"value": payload}}, upsert=True)

    def write_one(_: int) -> tuple[bool, dict[str, Any]]:
        key = uuid.uuid4().hex
        started = time.perf_counter()
        try:
            collection.insert_one({"_id": key, "value": payload, "ts": time.time()})
            return True, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    def read_one(i: int) -> tuple[bool, dict[str, Any]]:
        started = time.perf_counter()
        try:
            doc = collection.find_one({"_id": f"seed_{i % 200}"})
            return bool(doc) and len(doc.get("value", "")) == 256, {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    return [summarize(run_parallel("mongo_insert_256b", total, concurrency, write_one)),
            summarize(run_parallel("mongo_findone_256b", total, concurrency, read_one))]


def http_json(method: str, url: str, payload: dict[str, Any] | None = None) -> dict[str, Any]:
    data = None if payload is None else json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"}, method=method)
    with urllib.request.urlopen(req, timeout=10) as resp:
        return json.loads(resp.read().decode("utf-8"))


def bench_qdrant(total: int, concurrency: int) -> list[dict[str, Any]]:
    collection = f"resume_bench_{int(time.time())}_{uuid.uuid4().hex[:6]}"
    http_json("PUT", f"http://127.0.0.1:6333/collections/{collection}", {"vectors": {"size": 16, "distance": "Cosine"}})
    points = []
    for i in range(200):
        points.append({"id": i + 1, "vector": [random.random() for _ in range(16)], "payload": {"kind": "seed"}})
    http_json("PUT", f"http://127.0.0.1:6333/collections/{collection}/points?wait=true", {"points": points})

    def write_one(i: int) -> tuple[bool, dict[str, Any]]:
        started = time.perf_counter()
        try:
            point = {"id": 100000 + i, "vector": [random.random() for _ in range(16)], "payload": {"kind": "bench"}}
            data = http_json("PUT", f"http://127.0.0.1:6333/collections/{collection}/points?wait=true", {"points": [point]})
            return data.get("status") == "ok", {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    def search_one(_: int) -> tuple[bool, dict[str, Any]]:
        started = time.perf_counter()
        try:
            data = http_json("POST", f"http://127.0.0.1:6333/collections/{collection}/points/search", {"vector": [random.random() for _ in range(16)], "limit": 5, "with_payload": True})
            return data.get("status") == "ok", {"elapsed_ms": (time.perf_counter() - started) * 1000.0}
        except Exception as exc:
            return False, {"elapsed_ms": (time.perf_counter() - started) * 1000.0, "error": str(exc)}

    return [summarize(run_parallel("qdrant_upsert_16d", total, concurrency, write_one)),
            summarize(run_parallel("qdrant_vector_search_top5_16d", total, concurrency, search_one))]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--scenario", choices=["redis", "postgres", "mongo", "qdrant", "all"], default="all")
    parser.add_argument("--total", type=int, default=200)
    parser.add_argument("--concurrency", type=int, default=20)
    parser.add_argument("--report-dir", default="reports")
    args = parser.parse_args()
    report_dir = Path(args.report_dir)
    report_dir.mkdir(parents=True, exist_ok=True)

    scenarios = [args.scenario] if args.scenario != "all" else ["redis", "postgres", "mongo", "qdrant"]
    all_reports: list[dict[str, Any]] = []
    for scenario in scenarios:
        if scenario == "redis":
            reports = bench_redis(args.total, args.concurrency)
        elif scenario == "postgres":
            reports = bench_postgres(args.total, args.concurrency)
        elif scenario == "mongo":
            reports = bench_mongo(max(20, args.total // 4), max(1, args.concurrency // 2))
        elif scenario == "qdrant":
            reports = bench_qdrant(args.total, args.concurrency)
        else:
            raise ValueError(scenario)
        for report in reports:
            path = report_dir / f"{report['name']}_{time.strftime('%Y%m%d_%H%M%S', time.gmtime())}.json"
            path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
            print(json.dumps(report["summary"], ensure_ascii=False))
            print(f"Report: {path}")
        all_reports.extend(reports)

    summary_path = report_dir / f"system_bench_summary_{time.strftime('%Y%m%d_%H%M%S', time.gmtime())}.json"
    summary_path.write_text(json.dumps({"reports": all_reports}, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Summary: {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
