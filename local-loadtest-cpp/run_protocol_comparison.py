#!/usr/bin/env python3
"""
run_protocol_comparison.py -- One-shot TCP vs QUIC (vs HTTP) load-test comparison.

Runs all three protocol scenarios (TCP, QUIC, HTTP) in sequence using the C++
memochat_loadtest binary via cpp_run.py, then calls compare_protocols.py to
produce a side-by-side comparison report.

Usage:
    python run_protocol_comparison.py
    python run_protocol_comparison.py --total 1000 --concurrency 100
    python run_protocol_comparison.py --skip-quic --filter-accounts
    python run_protocol_comparison.py --scenarios tcp quic
"""

import argparse
import json
import os
import subprocess
import sys
import glob
from pathlib import Path
from datetime import datetime, timezone

SCRIPT_DIR = Path(__file__).parent.resolve()
CPP_RUN    = SCRIPT_DIR / "cpp_run.py"
COMPARE    = SCRIPT_DIR / "compare_protocols.py"


def load_config(config_path: str) -> dict:
    with open(config_path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_config(cfg: dict, path: str):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2, ensure_ascii=False)


def get_report_dir(config_path: str) -> Path:
    cfg = load_config(config_path)
    cfg_dir = Path(config_path).parent.resolve()
    rd = cfg.get("report_dir", "reports")
    if Path(rd).is_absolute():
        return Path(rd)
    return (cfg_dir / rd).resolve()


def ensure_accounts_csv(config_path: str) -> str:
    """Return the accounts CSV path from config, or die with a helpful message."""
    cfg = load_config(config_path)
    runtime_csv = cfg.get("runtime_accounts_csv", "")
    accounts_csv = cfg.get("accounts_csv", runtime_csv)
    if not accounts_csv:
        print("[run_protocol_comparison] ERROR: accounts_csv not found in config.json", file=sys.stderr)
        print("  Set accounts_csv or runtime_accounts_csv in config.json", file=sys.stderr)
        sys.exit(1)
    p = Path(accounts_csv)
    if not p.is_absolute():
        p = (Path(config_path).parent.resolve() / p).resolve()
    if not p.exists():
        print(f"[run_protocol_comparison] ERROR: accounts CSV not found: {p}", file=sys.stderr)
        sys.exit(1)
    return str(p)


def run_cpp_binary(scenario: str,
                   config_path: str,
                   accounts_csv: str,
                   total: int,
                   concurrency: int,
                   extra_args: list[str],
                   skip_if_no_binary: bool = False) -> bool:
    """
    Call cpp_run.py to invoke the C++ binary for one scenario.
    Returns True on success, False on failure.
    """
    args = [
        sys.executable,
        str(CPP_RUN),
        "--scenario", scenario,
        "--config", config_path,
        "--report-path", "",   # let cpp_run auto-detect
    ]
    if accounts_csv:
        # The C++ binary reads accounts_csv from config, not a CLI flag,
        # but we pass it so the caller knows it was validated
        pass

    for key, val in [
        ("--total",       total),
        ("--concurrency", concurrency),
    ]:
        if val is not None:
            args += [key, str(val)]

    for a in extra_args:
        args.append(a)

    if skip_if_no_binary:
        args.append("--skip-if-no-binary")

    print(f"\n{'='*60}")
    print(f"[run_protocol_comparison] Running {scenario.upper()} scenario")
    print(f"[run_protocol_comparison] {' '.join(args)}")
    print(f"{'='*60}\n")

    result = subprocess.run(args, capture_output=False, text=True)
    return result.returncode == 0


def main():
    parser = argparse.ArgumentParser(
        description="Run TCP, QUIC, and HTTP load tests in one shot and compare results.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "--config",
        default="config.json",
        help="Path to config.json (default: config.json in script directory)"
    )
    parser.add_argument(
        "--scenarios",
        nargs="+",
        default=["tcp", "quic", "http"],
        choices=["tcp", "quic", "http"],
        help="Which protocols to run (default: tcp quic http)"
    )
    parser.add_argument(
        "--total",
        type=int, default=None,
        help="Total requests per scenario (overrides config)"
    )
    parser.add_argument(
        "--concurrency",
        type=int, default=None,
        help="Concurrent workers per scenario (overrides config)"
    )
    parser.add_argument(
        "--warmup",
        type=int, default=None,
        help="Warmup iterations (overrides config)"
    )
    parser.add_argument(
        "--filter-accounts",
        action="store_true",
        help="Probe each account via HTTP before the run and keep only valid ones"
    )
    parser.add_argument(
        "--skip-quic",
        action="store_true",
        help="Skip QUIC scenario (useful when msquic is unavailable)"
    )
    parser.add_argument(
        "--compare-only",
        action="store_true",
        help="Skip running tests and only regenerate compare_protocols.json from existing reports"
    )
    parser.add_argument(
        "--output",
        default="compare_protocols.json",
        help="Output comparison report path"
    )

    args = parser.parse_args()

    # Resolve config path
    cfg_path = Path(args.config)
    if not cfg_path.is_absolute():
        cfg_path = (SCRIPT_DIR / cfg_path).resolve()
    if not cfg_path.exists():
        print(f"[run_protocol_comparison] ERROR: config not found: {cfg_path}", file=sys.stderr)
        sys.exit(1)

    # Ensure accounts CSV exists
    accounts_csv = ensure_accounts_csv(str(cfg_path))
    report_dir = get_report_dir(str(cfg_path))
    print(f"[run_protocol_comparison] Config : {cfg_path}")
    print(f"[run_protocol_comparison] Accounts: {accounts_csv}")
    print(f"[run_protocol_comparison] Reports : {report_dir}")

    extra = []
    if args.filter_accounts:
        extra.append("--filter-accounts")
    if args.warmup is not None:
        extra.append("--warmup")
        extra.append(str(args.warmup))

    results = {}
    scenarios = args.scenarios
    if args.skip_quic and "quic" in scenarios:
        scenarios = [s for s in scenarios if s != "quic"]

    if args.compare_only:
        print("[run_protocol_comparison] --compare-only: skipping test runs")

        # Just run comparison on existing reports
        result = subprocess.run([
            sys.executable, str(COMPARE),
            "--input-dir", str(report_dir),
            "--output", args.output
        ], capture_output=False)
        sys.exit(result.returncode)

    for scenario in scenarios:
        success = run_cpp_binary(
            scenario=scenario,
            config_path=str(cfg_path),
            accounts_csv=accounts_csv,
            total=args.total,
            concurrency=args.concurrency,
            extra_args=extra,
            skip_if_no_binary=False,
        )
        results[scenario] = "OK" if success else "FAILED"
        if not success:
            print(f"[run_protocol_comparison] WARNING: {scenario} scenario failed", file=sys.stderr)

    # Run comparison
    print(f"\n{'='*60}")
    print(f"[run_protocol_comparison] Generating comparison report")
    print(f"{'='*60}\n")

    tcp_paths  = [str(p) for p in sorted(report_dir.glob("tcp_login_*.json"))]
    quic_paths = [str(p) for p in sorted(report_dir.glob("quic_login_*.json"))]
    http_paths = [str(p) for p in sorted(report_dir.glob("http_login_*.json"))]

    cmp_args = ["--output", args.output]
    for p in tcp_paths:
        cmp_args += ["--tcp-report", p]
    for p in quic_paths:
        cmp_args += ["--quic-report", p]
    for p in http_paths:
        cmp_args += ["--http-report", p]

    if not tcp_paths and not quic_paths and not http_paths:
        print("[run_protocol_comparison] ERROR: No report files found in", report_dir, file=sys.stderr)
        sys.exit(1)

    cmp_result = subprocess.run([sys.executable, str(COMPARE)] + cmp_args, capture_output=False)

    print(f"\n[run_protocol_comparison] Results: {results}")
    print(f"[run_protocol_comparison] Comparison: {args.output}")
    sys.exit(cmp_result.returncode)


if __name__ == "__main__":
    main()
