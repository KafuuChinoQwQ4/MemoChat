#!/usr/bin/env python3
"""Run MemoChat load tests as separate application and dependency layers."""

from __future__ import annotations

import argparse
import json
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any


APP_CASES = ["http-health", "http-login", "tcp-login", "tcp-heartbeat", "tcp-relation-bootstrap"]
DB_CASES = ["redis", "postgres", "mongo", "qdrant"]
AI_CASES = ["ai-health"]
DEEP_AI_CASES = ["agent", "rag"]
DEFAULT_CASES = APP_CASES + DB_CASES + AI_CASES


@dataclass
class CaseRun:
    case: str
    layer: str
    command: list[str]
    report_path: Path | None = None
    exit_code: int = 0
    stdout: str = ""
    stderr: str = ""
    elapsed_sec: float = 0.0


def now_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def resolve_path(path: str, base_dir: Path) -> Path:
    candidate = Path(path)
    if candidate.is_absolute():
        return candidate
    return (base_dir / candidate).resolve()


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}


def summarize_latency(report: dict[str, Any]) -> dict[str, float]:
    return report.get("summary", {}).get("latency_ms", {}) or {}


def report_total(report: dict[str, Any]) -> int:
    summary = report.get("summary", {})
    if "total" in summary:
        return int(summary.get("total") or 0)
    if "total_runs" in summary:
        return int(summary.get("total_runs") or 0)
    return 0


def report_failed(report: dict[str, Any]) -> int:
    return int(report.get("summary", {}).get("failed") or 0)


def normalize_report(
    report: dict[str, Any],
    *,
    case: str,
    layer: str,
    run: CaseRun | None = None,
) -> dict[str, Any]:
    scenario = str(report.get("scenario") or report.get("name") or case)
    errors = report.get("errors")
    if errors is None:
        errors = report.get("details", {}).get("errors", {})
    normalized = {
        "case": case,
        "layer": layer,
        "scenario": scenario,
        "started_at": report.get("started_at", ""),
        "finished_at": report.get("finished_at", ""),
        "status": "PASS" if report_failed(report) == 0 else "FAIL",
        "summary": report.get("summary", {}),
        "errors": errors or {},
        "quality": report.get("quality", {}),
        "details": report.get("details", {}),
    }
    if run is not None:
        normalized["runner"] = {
            "command": " ".join(run.command),
            "exit_code": run.exit_code,
            "elapsed_sec": round(run.elapsed_sec, 4),
            "report_path": str(run.report_path or ""),
            "stdout_tail": run.stdout[-4000:],
            "stderr_tail": run.stderr[-4000:],
        }
        if run.exit_code != 0:
            normalized["status"] = "FAIL"
            normalized["errors"] = dict(normalized["errors"])
            normalized["errors"][f"runner_exit_{run.exit_code}"] = 1
    return normalized


def synthetic_failure(case: str, layer: str, error: str, run: CaseRun | None = None) -> dict[str, Any]:
    started = now_iso()
    report = {
        "case": case,
        "layer": layer,
        "scenario": case,
        "started_at": started,
        "finished_at": started,
        "status": "FAIL",
        "summary": {
            "total": 0,
            "success": 0,
            "failed": 1,
            "success_rate": 0.0,
            "wall_elapsed_sec": round(run.elapsed_sec, 4) if run else 0.0,
            "throughput_rps": 0.0,
            "latency_ms": {
                "min_ms": 0.0,
                "avg_ms": 0.0,
                "p50_ms": 0.0,
                "p75_ms": 0.0,
                "p90_ms": 0.0,
                "p95_ms": 0.0,
                "p99_ms": 0.0,
                "max_ms": 0.0,
            },
        },
        "errors": {error[:160]: 1},
        "quality": {},
        "details": {},
    }
    if run is not None:
        report["runner"] = {
            "command": " ".join(run.command),
            "exit_code": run.exit_code,
            "elapsed_sec": round(run.elapsed_sec, 4),
            "report_path": str(run.report_path or ""),
            "stdout_tail": run.stdout[-4000:],
            "stderr_tail": run.stderr[-4000:],
        }
    return report


def run_subprocess(run: CaseRun, timeout_sec: int) -> CaseRun:
    started = time.perf_counter()
    try:
        proc = subprocess.run(
            run.command,
            cwd=str(repo_root()),
            capture_output=True,
            text=True,
            timeout=timeout_sec,
        )
        run.exit_code = proc.returncode
        run.stdout = proc.stdout
        run.stderr = proc.stderr
    except subprocess.TimeoutExpired as exc:
        run.exit_code = 124
        run.stdout = exc.stdout or ""
        run.stderr = (exc.stderr or "") + f"\nTimeout after {timeout_sec}s"
    run.elapsed_sec = time.perf_counter() - started
    return run


def run_k6_case(
    case: str,
    scenario: str,
    config_path: Path,
    report_dir: Path,
    total: int,
    concurrency: int,
    timeout_sec: int,
) -> list[dict[str, Any]]:
    summary_path = report_dir / f"{case}_{time.strftime('%Y%m%d_%H%M%S', time.gmtime())}.json"
    wrapper = repo_root() / "tools" / "loadtest" / "k6" / "run-http-bench.ps1"
    command = [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(wrapper),
        "-Scenario",
        scenario,
        "-ConfigPath",
        str(config_path),
        "-Total",
        str(total),
        "-Concurrency",
        str(concurrency),
        "-SummaryPath",
        str(summary_path),
    ]
    run = run_subprocess(CaseRun(case, "app", command, summary_path), timeout_sec)
    if summary_path.exists():
        report = load_json(summary_path)
        return [normalize_report(report, case=case, layer="app", run=run)]
    error = run.stderr.strip() or run.stdout.strip() or f"{case} did not produce a k6 summary"
    return [synthetic_failure(case, "app", error, run)]


def run_py_case(
    case: str,
    py_scenario: str,
    config_path: Path,
    report_dir: Path,
    total: int,
    concurrency: int,
    timeout_sec: int,
) -> list[dict[str, Any]]:
    script = repo_root() / "tools" / "loadtest" / "python-loadtest" / "py_loadtest.py"
    before = set(report_dir.glob("*.json"))
    command = [
        sys.executable,
        str(script),
        "--scenario",
        py_scenario,
        "--config",
        str(config_path),
        "--total",
        str(total),
        "--concurrency",
        str(concurrency),
        "--report-dir",
        str(report_dir),
    ]
    run = run_subprocess(CaseRun(case, "app", command), timeout_sec)
    after = set(report_dir.glob("*.json"))
    created = sorted(after - before, key=lambda item: item.stat().st_mtime, reverse=True)
    if created:
        run.report_path = created[0]
        report = load_json(created[0])
        return [normalize_report(report, case=case, layer="app", run=run)]
    error = run.stderr.strip() or run.stdout.strip() or f"{case} did not produce a Python load-test report"
    return [synthetic_failure(case, "app", error, run)]


def run_db_case(case: str, total: int, concurrency: int) -> list[dict[str, Any]]:
    started = time.perf_counter()
    try:
        import system_bench

        if case == "redis":
            reports = system_bench.bench_redis(total, concurrency)
        elif case == "postgres":
            reports = system_bench.bench_postgres(total, concurrency)
        elif case == "mongo":
            reports = system_bench.bench_mongo(max(20, total // 4), max(1, concurrency // 2))
        elif case == "qdrant":
            reports = system_bench.bench_qdrant(total, concurrency)
        else:
            raise ValueError(f"unsupported db case: {case}")
    except Exception as exc:
        run = CaseRun(case, "dependency", [], elapsed_sec=time.perf_counter() - started)
        return [synthetic_failure(case, "dependency", str(exc), run)]
    normalized = []
    for report in reports:
        normalized.append(normalize_report(report, case=case, layer="dependency"))
    return normalized


def tcp_port_open(host: str, port: int, timeout: float = 0.4) -> bool:
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


def environment_snapshot() -> dict[str, Any]:
    checks = {
        "gate_8080": tcp_port_open("127.0.0.1", 8080),
        "gate_8084": tcp_port_open("127.0.0.1", 8084),
        "ai_orchestrator_8096": tcp_port_open("127.0.0.1", 8096),
        "redis_6379": tcp_port_open("127.0.0.1", 6379),
        "postgres_15432": tcp_port_open("127.0.0.1", 15432),
        "mongo_27017": tcp_port_open("127.0.0.1", 27017),
        "qdrant_6333": tcp_port_open("127.0.0.1", 6333),
        "rabbitmq_5672": tcp_port_open("127.0.0.1", 5672),
        "redpanda_19092": tcp_port_open("127.0.0.1", 19092),
    }
    docker = subprocess.run(
        ["docker", "ps", "--format", "{{.Names}}\t{{.Status}}\t{{.Ports}}"],
        cwd=str(repo_root()),
        capture_output=True,
        text=True,
    )
    return {
        "tcp_ports": checks,
        "docker_ps_exit_code": docker.returncode,
        "docker_ps": docker.stdout[-12000:],
        "docker_ps_stderr": docker.stderr[-4000:],
    }


def parse_cases(raw: str, include_deep_ai: bool) -> list[str]:
    if raw.strip().lower() == "default":
        cases = list(DEFAULT_CASES)
    elif raw.strip().lower() == "all":
        cases = list(DEFAULT_CASES)
        if include_deep_ai:
            cases += DEEP_AI_CASES
    else:
        cases = [item.strip().lower() for item in raw.split(",") if item.strip()]
    valid = set(APP_CASES + DB_CASES + AI_CASES + DEEP_AI_CASES)
    unknown = [case for case in cases if case not in valid]
    if unknown:
        raise SystemExit(f"unknown case(s): {', '.join(unknown)}")
    return cases


def bottleneck_hints(reports: list[dict[str, Any]], environment: dict[str, Any]) -> list[str]:
    hints: list[str] = []
    by_scenario = {str(report.get("scenario")): report for report in reports}
    by_case = {}
    for report in reports:
        by_case.setdefault(str(report.get("case")), []).append(report)

    ports = environment.get("tcp_ports", {})
    if not ports.get("gate_8080") and not ports.get("gate_8084"):
        hints.append("GateServer 8080/8084 is not reachable, so HTTP/TCP business benchmarks will measure environment failure.")

    health = by_scenario.get("http_health_k6")
    http = by_scenario.get("http_login_k6")
    tcp = by_scenario.get("tcp_login")
    if health and report_failed(health) == 0 and http and report_failed(http) == 0:
        h_rps = float(health["summary"].get("throughput_rps", 0.0))
        l_rps = float(http["summary"].get("throughput_rps", 0.0))
        if h_rps > 0 and l_rps < h_rps * 0.5:
            hints.append("HTTP login throughput is below 50% of HTTP health baseline; login-side DB/cache/auth logic is likely dominating.")
    if http and tcp and report_failed(http) == 0 and report_failed(tcp) == 0:
        h_rps = float(http["summary"].get("throughput_rps", 0.0))
        t_rps = float(tcp["summary"].get("throughput_rps", 0.0))
        if h_rps > 0 and t_rps < h_rps * 0.7:
            hints.append("TCP full login is much lower than HTTP login; ChatServer auth, TCP connection churn, or status side effects need inspection.")

    for report in reports:
        latency = summarize_latency(report)
        p95 = float(latency.get("p95_ms", 0.0) or 0.0)
        p99 = float(latency.get("p99_ms", 0.0) or 0.0)
        if p95 > 0 and p99 > p95 * 2:
            hints.append(f"{report.get('scenario')} has a wide p95->p99 tail ({p95}ms -> {p99}ms); check queueing, connection pools, and slow dependency calls.")

    dependency_reports = [report for report in reports if report.get("layer") == "dependency" and report_failed(report) == 0]
    slow_deps = sorted(
        dependency_reports,
        key=lambda report: float(summarize_latency(report).get("p99_ms", 0.0) or 0.0),
        reverse=True,
    )[:3]
    if slow_deps:
        text = ", ".join(
            f"{item.get('scenario')} p99={summarize_latency(item).get('p99_ms', 0)}ms"
            for item in slow_deps
        )
        hints.append(f"Highest dependency tail latencies: {text}.")
    return hints


def write_markdown_summary(path: Path, suite: dict[str, Any]) -> None:
    lines = [
        "# MemoChat Layered Benchmark Summary",
        "",
        f"- Started: {suite['started_at']}",
        f"- Finished: {suite['finished_at']}",
        f"- Report: `{suite['report_path']}`",
        "",
        "| Layer | Case | Scenario | Status | Total | Success | Failed | Success % | RPS | Min | Avg | P50 | P75 | P90 | P95 | P99 | Max |",
        "|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for report in suite["reports"]:
        summary = report.get("summary", {})
        latency = summary.get("latency_ms", {}) or {}
        lines.append(
            "| {layer} | {case} | {scenario} | {status} | {total} | {success} | {failed} | {success_rate} | {rps} | {min_ms} | {avg_ms} | {p50} | {p75} | {p90} | {p95} | {p99} | {max_ms} |".format(
                layer=report.get("layer", ""),
                case=report.get("case", ""),
                scenario=report.get("scenario", ""),
                status=report.get("status", ""),
                total=summary.get("total", summary.get("total_runs", 0)),
                success=summary.get("success", ""),
                failed=summary.get("failed", ""),
                success_rate=round(float(summary.get("success_rate", 0.0)) * 100, 2) if "success_rate" in summary else "",
                rps=summary.get("throughput_rps", summary.get("best_throughput_rps", "")),
                min_ms=latency.get("min_ms", ""),
                avg_ms=latency.get("avg_ms", ""),
                p50=latency.get("p50_ms", ""),
                p75=latency.get("p75_ms", ""),
                p90=latency.get("p90_ms", ""),
                p95=latency.get("p95_ms", ""),
                p99=latency.get("p99_ms", summary.get("worst_p99_ms", "")),
                max_ms=latency.get("max_ms", ""),
            )
        )
    failing_errors = []
    for report in suite["reports"]:
        errors = report.get("errors", {})
        if errors:
            failing_errors.append((report.get("scenario", ""), errors))
    if failing_errors:
        lines += ["", "## Error Buckets", ""]
        for scenario, errors in failing_errors:
            lines.append(f"### {scenario}")
            for key, count in errors.items():
                lines.append(f"- `{key}`: {count}")
    if suite.get("bottleneck_hints"):
        lines += ["", "## Bottleneck Hints", ""]
        lines += [f"- {hint}" for hint in suite["bottleneck_hints"]]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run split MemoChat application and dependency benchmarks")
    parser.add_argument("--config", default=str(Path(__file__).with_name("config.benchmark.json")))
    parser.add_argument("--cases", default="default", help="default, all, or comma list: http-health,http-login,tcp-login,tcp-heartbeat,tcp-relation-bootstrap,redis,postgres,mongo,qdrant,ai-health,agent,rag")
    parser.add_argument("--total", type=int, default=200)
    parser.add_argument("--concurrency", type=int, default=20)
    parser.add_argument("--db-total", type=int, default=0)
    parser.add_argument("--db-concurrency", type=int, default=0)
    parser.add_argument("--report-dir", default="")
    parser.add_argument("--timeout-sec", type=int, default=600)
    parser.add_argument("--include-deep-ai", action="store_true")
    parser.add_argument("--heavy", action="store_true", help="Use a larger default run: 2000 total, 100 concurrency")
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    config = load_json(config_path)
    config_dir = config_path.parent
    report_dir = Path(args.report_dir).resolve() if args.report_dir else resolve_path(
        str(config.get("report_dir", "../../../infra/Memo_ops/artifacts/loadtest/runtime/reports")),
        config_dir,
    )
    report_dir.mkdir(parents=True, exist_ok=True)

    total = max(1, args.total)
    concurrency = max(1, args.concurrency)
    if args.heavy and args.total == 200 and args.concurrency == 20:
        total = 2000
        concurrency = 100
    db_total = max(1, args.db_total or total)
    db_concurrency = max(1, args.db_concurrency or concurrency)
    cases = parse_cases(args.cases, args.include_deep_ai)

    started_at = now_iso()
    environment = environment_snapshot()
    reports: list[dict[str, Any]] = []

    for case in cases:
        print(f"[RUN] {case} total={total if case not in DB_CASES else db_total} concurrency={concurrency if case not in DB_CASES else db_concurrency}")
        if case == "http-health":
            reports.extend(run_k6_case(case, "health", config_path, report_dir, total, concurrency, args.timeout_sec))
        elif case == "http-login":
            reports.extend(run_k6_case(case, "login", config_path, report_dir, total, concurrency, args.timeout_sec))
        elif case == "tcp-login":
            reports.extend(run_py_case(case, "tcp", config_path, report_dir, total, concurrency, args.timeout_sec))
        elif case == "tcp-heartbeat":
            reports.extend(run_py_case(case, "tcp-heartbeat", config_path, report_dir, total, concurrency, args.timeout_sec))
        elif case == "tcp-relation-bootstrap":
            reports.extend(run_py_case(case, "tcp-relation-bootstrap", config_path, report_dir, total, concurrency, args.timeout_sec))
        elif case == "ai-health":
            reports.extend(run_py_case(case, "ai-health", config_path, report_dir, total, concurrency, args.timeout_sec))
        elif case in {"agent", "rag"}:
            slow_total = min(total, 20)
            slow_concurrency = min(concurrency, 4)
            reports.extend(run_py_case(case, case, config_path, report_dir, slow_total, slow_concurrency, args.timeout_sec))
        elif case in DB_CASES:
            reports.extend(run_db_case(case, db_total, db_concurrency))
        else:
            reports.append(synthetic_failure(case, "unknown", "unsupported case"))

    finished_at = now_iso()
    suite_path = report_dir / f"layered_suite_{time.strftime('%Y%m%d_%H%M%S', time.gmtime())}.json"
    suite: dict[str, Any] = {
        "scenario": "layered_suite",
        "started_at": started_at,
        "finished_at": finished_at,
        "config_path": str(config_path),
        "report_path": str(suite_path),
        "parameters": {
            "cases": cases,
            "total": total,
            "concurrency": concurrency,
            "db_total": db_total,
            "db_concurrency": db_concurrency,
            "timeout_sec": args.timeout_sec,
        },
        "environment": environment,
        "reports": reports,
    }
    suite["bottleneck_hints"] = bottleneck_hints(reports, environment)
    suite_path.write_text(json.dumps(suite, ensure_ascii=False, indent=2), encoding="utf-8")
    md_path = suite_path.with_suffix(".md")
    write_markdown_summary(md_path, suite)
    print(f"Suite report: {suite_path}")
    print(f"Markdown summary: {md_path}")
    return 1 if any(report.get("status") == "FAIL" for report in reports) else 0


if __name__ == "__main__":
    raise SystemExit(main())
