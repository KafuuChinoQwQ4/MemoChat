#!/usr/bin/env python3
"""
cpp_run.py -- Wrapper script that invokes the C++ memochat_loadtest binary.

Accepts the same arguments as the C++ binary (--config, --scenario, --total,
--concurrency, --warmup, --filter-accounts), plus a few conveniences:

  --cpp-binary <path>   Path to memochat_loadtest.exe (default: auto-detect)
  --report-path <path>   Output JSON report path (overrides default naming)
  --skip-if-no-binary   Exit 0 if the binary is not found (silent skip)

The script reads the JSON report produced by the C++ binary and re-emits it
in the same JSON-line log format as the Python loadtest scripts, so that
run_suite.ps1 and the ELK-compatible log pipeline can treat them uniformly.

Example usage:

    python cpp_run.py --scenario tcp --total 500 --concurrency 50 --config config.json
    python cpp_run.py --scenario quic --filter-accounts --config config.json
    python cpp_run.py --scenario http --report-path reports/my_http.json --config config.json
"""

import argparse
import json
import os
import subprocess
import sys
import glob
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Auto-detection helpers
# ---------------------------------------------------------------------------

def find_cpp_binary():
    """
    Search relative to this script's directory:
      - build/Release/memochat_loadtest.exe          (local dev build)
      - ../../build/Release/memochat_loadtest.exe     (when invoked from suite)
    Falls back to PATH.
    """
    script_dir = Path(__file__).parent.resolve()
    candidates = [
        script_dir / "build" / "Release" / "memochat_loadtest.exe",
        script_dir.parent / "build" / "Release" / "memochat_loadtest.exe",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate)

    # Check PATH
    for name in ["memochat_loadtest.exe", "memochat_loadtest"]:
        for base in os.environ.get("PATH", "").split(os.pathsep):
            p = Path(base) / name
            if p.is_file():
                return str(p)
    return None


def detect_report_path(scenario: str, config_path: str) -> str:
    """
    Return the same path that the C++ binary would produce, so we can read
    it after the binary exits.  The binary writes to:
      <report_dir>/<scenario>_login_<timestamp>.json
    where report_dir comes from the config file (or defaults to "reports").
    """
    cfg_dir = Path(config_path).parent.resolve()
    report_dir = cfg_dir / "reports"

    # Try to read report_dir from config
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            cfg = json.load(f)
        rd = cfg.get("report_dir", "reports")
        if not Path(rd).is_absolute():
            report_dir = (cfg_dir / rd).resolve()
    except Exception:
        pass

    report_dir.mkdir(parents=True, exist_ok=True)
    ts = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    return str(report_dir / f"{scenario}_login_{ts}.json")


def find_latest_report(scenario, report_dir):
    """Find the most recent report matching <scenario>_login_*.json."""
    pattern = f"{scenario}_login_*.json"
    candidates = sorted(report_dir.glob(pattern), key=lambda p: p.stat().st_mtime)
    return candidates[-1] if candidates else None


# ---------------------------------------------------------------------------
# Report conversion
# ---------------------------------------------------------------------------

def convert_cpp_report(cpp_report_path: str) -> dict:
    """
    Read the C++ JSON report and normalise it to the same schema as the
    Python loadtest reports so that run_suite.ps1 and downstream tooling
    can treat both sources uniformly.

    C++ schema fields we normalise:
      "scenario"          → "scenario"
      "total"             → "summary.total"
      "success"           → "summary.success"
      "failed"            → "summary.failed"
      "success_rate"      → "summary.success_rate"
      "rps"               → "summary.throughput_rps"
      "latency_ms"        → "summary.latency_ms"  (min/avg/p50/…/max)
      "phase_breakdown"   → "phase_breakdown"
      "top_errors"        → "top_errors"
    """
    with open(cpp_report_path, "r", encoding="utf-8") as f:
        cpp = json.load(f)

    latency = cpp.get("latency_ms", {})
    phase_bd = cpp.get("phase_breakdown", {})

    # top_errors is a flat list of [key, count] pairs in C++ output
    top_errors = cpp.get("top_errors", [])

    converted = {
        "scenario": cpp.get("scenario", "unknown"),
        "test": cpp.get("scenario", "unknown"),
        "summary": {
            "total":   cpp.get("total", 0),
            "success": cpp.get("success", 0),
            "failed":  cpp.get("failed", 0),
            "success_rate": cpp.get("success_rate", 0.0),
            "throughput_rps": cpp.get("rps", 0.0),
            "elapsed_sec": cpp.get("elapsed_sec", 0.0),
            "latency_ms": {
                "min":  latency.get("min", 0.0),
                "avg":  latency.get("avg", 0.0),
                "p50":  latency.get("p50", 0.0),
                "p75":  latency.get("p75", 0.0),
                "p90":  latency.get("p90", 0.0),
                "p95":  latency.get("p95", 0.0),
                "p99":  latency.get("p99", 0.0),
                "p999": latency.get("p999", 0.0),
                "max":  latency.get("max", 0.0),
            }
        },
        "phase_breakdown": {
            name: {
                "min":  s.get("min", 0.0),
                "avg":  s.get("avg", 0.0),
                "p50":  s.get("p50", 0.0),
                "p75":  s.get("p75", 0.0),
                "p90":  s.get("p90", 0.0),
                "p95":  s.get("p95", 0.0),
                "p99":  s.get("p99", 0.0),
                "max":  s.get("max", 0.0),
            }
            for name, s in phase_bd.items()
        },
        "top_errors": top_errors,
        "preconditions": [],
        "data_mutation_summary": [],
        "report_path": cpp_report_path,
        "cpp_binary_report": cpp,
    }
    return converted


# ---------------------------------------------------------------------------
# CLI argument passthrough
# ---------------------------------------------------------------------------

def build_cpp_args(args):
    """Translate our Namespace into the list that the C++ binary expects."""
    cpp_args = []

    if args.config:
        cpp_args += ["--config", args.config]

    if args.scenario:
        cpp_args += ["--scenario", args.scenario]

    if args.total is not None:
        cpp_args += ["--total", str(args.total)]

    if args.concurrency is not None:
        cpp_args += ["--concurrency", str(args.concurrency)]

    if args.warmup is not None:
        cpp_args += ["--warmup", str(args.warmup)]

    if args.retries is not None:
        cpp_args += ["--retries", str(args.retries)]

    if args.filter_accounts:
        cpp_args.append("--filter-accounts")

    return cpp_args


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Invoke the C++ memochat_loadtest binary with optional report conversion.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "--cpp-binary",
        default=None,
        help="Path to memochat_loadtest.exe (auto-detected if omitted)"
    )
    parser.add_argument(
        "--report-path",
        default=None,
        help="Where to write the converted Python-compatible report JSON"
    )
    parser.add_argument(
        "--skip-if-no-binary",
        action="store_true",
        help="Exit 0 (instead of 1) when the C++ binary cannot be found"
    )
    parser.add_argument(
        "--config",
        default="config.json",
        help="Path to config.json (default: config.json in script dir)"
    )
    parser.add_argument(
        "--scenario",
        default="tcp",
        choices=["tcp", "quic", "http"],
        help="Protocol scenario (default: tcp)"
    )
    parser.add_argument(
        "--total",
        type=int, default=None,
        help="Total requests (overrides config)"
    )
    parser.add_argument(
        "--concurrency",
        type=int, default=None,
        help="Concurrent workers (overrides config)"
    )
    parser.add_argument(
        "--warmup",
        type=int, default=None,
        help="Warmup iterations (overrides config)"
    )
    parser.add_argument(
        "--retries",
        type=int, default=None,
        help="Max retries per request (overrides config)"
    )
    parser.add_argument(
        "--filter-accounts",
        action="store_true",
        help="HTTP-probe each account before the run and keep only valid ones"
    )
    parser.add_argument(
        "--echo",
        action="store_true",
        help="Echo the C++ binary command line instead of running it"
    )

    args = parser.parse_args()

    # Locate binary
    binary = args.cpp_binary or find_cpp_binary()
    if not binary:
        if args.skip_if_no_binary:
            print("[cpp_run] C++ binary not found — skipping (--skip-if-no-binary)")
            sys.exit(0)
        print("[cpp_run] ERROR: memochat_loadtest.exe not found.", file=sys.stderr)
        print("  Searched: build/Release/, relative to this script".replace("  ", ""), file=sys.stderr)
        sys.exit(1)

    # Default config to script directory
    if args.config == "config.json":
        script_dir = Path(__file__).parent.resolve()
        candidate = script_dir / "config.json"
        if candidate.is_file():
            args.config = str(candidate)

    # Detect report output path if not specified
    if args.report_path:
        output_report = args.report_path
    else:
        output_report = detect_report_path(args.scenario, args.config)

    cpp_args = build_cpp_args(args)

    print(f"[cpp_run] Binary: {binary}")
    print(f"[cpp_run] Args  : {' '.join(cpp_args)}")

    if args.echo:
        print(" ".join([binary] + cpp_args))
        sys.exit(0)

    # Run the C++ binary (inherit stdout/stderr so output goes directly to terminal)
    try:
        completed = subprocess.run(
            [binary] + cpp_args,
            timeout=600
        )
    except subprocess.TimeoutExpired:
        print("[cpp_run] ERROR: C++ binary timed out after 600 seconds", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print(f"[cpp_run] ERROR: binary not found: {binary}", file=sys.stderr)
        sys.exit(1)

    # Find the report the C++ binary just wrote
    cfg_dir = Path(args.config).parent.resolve()
    try:
        with open(args.config, "r", encoding="utf-8") as f:
            cfg = json.load(f)
        rd = cfg.get("report_dir", "reports")
        report_dir = (cfg_dir / rd).resolve() if not Path(rd).is_absolute() else Path(rd)
    except Exception:
        report_dir = cfg_dir / "reports"

    # The binary writes a timestamped report; find the newest one
    cpp_report = find_latest_report(args.scenario, report_dir)
    if not cpp_report:
        # Fallback: user specified --report-path and the binary wrote there
        if args.report_path:
            cpp_report = Path(args.report_path)

    if cpp_report and cpp_report.exists():
        converted = convert_cpp_report(str(cpp_report))

        # Always write the Python-compatible report to the path run_suite.ps1 expects
        with open(output_report, "w", encoding="utf-8") as f:
            json.dump(converted, f, indent=2, ensure_ascii=False)

        print(f"[cpp_run] Report: {output_report}")
    else:
        print("[cpp_run] WARNING: Could not locate C++ report JSON — suite summary may be incomplete",
              file=sys.stderr)

    sys.exit(completed.returncode)


if __name__ == "__main__":
    main()
