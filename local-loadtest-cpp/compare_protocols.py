#!/usr/bin/env python3
"""
compare_protocols.py -- Generate a comparison report from TCP and QUIC
load-test JSON reports produced by the C++ binary (or Python scripts).

Reads one or more TCP login report paths and one or more QUIC login report
paths, computes aggregated statistics across all runs of each protocol, and
writes a comparison JSON to --output (default: compare_protocols.json).

Usage:
    python compare_protocols.py --tcp-report tcp_login_*.json --quic-report quic_login_*.json
    python compare_protocols.py --tcp-report tcp_*.json --quic-report quic_*.json --output compare.json
    python compare_protocols.py --input-dir ../Memo_ops/artifacts/loadtest/runtime/reports
"""

import argparse
import json
import glob
import os
import sys
from pathlib import Path
from datetime import datetime, timezone
from typing import Any


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    sorted_vals = sorted(values)
    idx = int((len(sorted_vals) - 1) * p)
    idx = max(0, min(idx, len(sorted_vals) - 1))
    return sorted_vals[idx]


def aggregate_reports(paths: list[str]) -> dict[str, Any]:
    """
    Merge multiple JSON reports into one aggregated summary.
    The individual reports can be either:
      - C++ binary format:  { total, success, failed, rps, elapsed_sec, latency_ms{avg,p50,p95,p99,max}, phase_breakdown{...} }
      - Python suite format: { summary: { total, success, failed, throughput_rps, latency_ms{avg,p50,p95,p99,max} } }
    Returns a dict with averaged / aggregated fields.
    """
    reports = []
    for p in paths:
        with open(p, "r", encoding="utf-8") as f:
            data = json.load(f)

        # Normalise C++ binary format
        if "summary" in data:
            # Python suite format
            s = data["summary"]
            latency = s.get("latency_ms", {})
            reports.append({
                "path": p,
                "total":     s.get("total", 0),
                "success":   s.get("success", 0),
                "failed":    s.get("failed", 0),
                "rps":       s.get("throughput_rps", 0.0),
                "elapsed_sec": s.get("elapsed_sec", 0.0),
                "latency_avg": latency.get("avg", latency.get("avg_ms", 0.0)),
                "latency_p50": latency.get("p50", 0.0),
                "latency_p95": latency.get("p95", 0.0),
                "latency_p99": latency.get("p99", 0.0),
                "latency_max": latency.get("max", 0.0),
                "phase_breakdown": data.get("phase_breakdown", {}),
            })
        else:
            # C++ binary format
            latency = data.get("latency_ms", {})
            reports.append({
                "path": p,
                "total":     data.get("total", 0),
                "success":   data.get("success", 0),
                "failed":    data.get("failed", 0),
                "rps":       data.get("rps", 0.0),
                "elapsed_sec": data.get("elapsed_sec", 0.0),
                "latency_avg": latency.get("avg", 0.0),
                "latency_p50": latency.get("p50", 0.0),
                "latency_p95": latency.get("p95", 0.0),
                "latency_p99": latency.get("p99", 0.0),
                "latency_max": latency.get("max", 0.0),
                "phase_breakdown": data.get("phase_breakdown", {}),
            })

    if not reports:
        return {}

    # Aggregate
    all_totals   = [r["total"]     for r in reports]
    all_success  = [r["success"]   for r in reports]
    all_failed   = [r["failed"]    for r in reports]
    all_rps      = [r["rps"]       for r in reports]
    all_elapsed  = [r["elapsed_sec"] for r in reports]
    all_avg      = [r["latency_avg"] for r in reports]
    all_p50      = [r["latency_p50"] for r in reports]
    all_p95      = [r["latency_p95"] for r in reports]
    all_p99      = [r["latency_p99"] for r in reports]
    all_max      = [r["latency_max"] for r in reports]

    def _mean(vals):
        return sum(vals) / len(vals) if vals else 0.0

    return {
        "runs": len(reports),
        "total": sum(all_totals),
        "total_success": sum(all_success),
        "total_failed": sum(all_failed),
        "success_rate": (sum(all_success) / sum(all_totals)) if sum(all_totals) else 0.0,
        "rps": _mean(all_rps),
        "elapsed_sec": _mean(all_elapsed),
        "latency_avg": _mean(all_avg),
        "latency_p50": _mean(all_p50),
        "latency_p95": _mean(all_p95),
        "latency_p99": _mean(all_p99),
        "latency_max": _mean(all_max),
        "rps_min": min(all_rps),
        "rps_max": max(all_rps),
        "latency_avg_min": min(all_avg),
        "latency_avg_max": max(all_avg),
        "paths": [r["path"] for r in reports],
        "phase_breakdown": reports[0].get("phase_breakdown", {}),
    }


def main():
    parser = argparse.ArgumentParser(description="Compare TCP vs QUIC load-test results.")
    parser.add_argument(
        "--tcp-report",
        nargs="+",
        default=[],
        help="One or more TCP report JSON files (supports glob patterns, e.g. tcp_login_*.json)"
    )
    parser.add_argument(
        "--quic-report",
        nargs="+",
        default=[],
        help="One or more QUIC report JSON files"
    )
    parser.add_argument(
        "--http-report",
        nargs="+",
        default=[],
        help="One or more HTTP report JSON files"
    )
    parser.add_argument(
        "--input-dir",
        default=None,
        help="Directory containing report files (auto-detects protocol from filename)"
    )
    parser.add_argument(
        "--output",
        default="compare_protocols.json",
        help="Output JSON file (default: compare_protocols.json)"
    )
    args = parser.parse_args()

    # Expand globs
    def expand(paths: list[str]) -> list[str]:
        result = []
        for p in paths:
            result.extend(glob.glob(p, recursive=False) or [p])
        return result

    # Auto-detect from input-dir
    if args.input_dir:
        dir_path = Path(args.input_dir)
        if not dir_path.is_dir():
            print(f"[compare_protocols] ERROR: not a directory: {args.input_dir}", file=sys.stderr)
            sys.exit(1)
        tcp_files   = sorted(dir_path.glob("tcp_login_*.json"))
        quic_files  = sorted(dir_path.glob("quic_login_*.json"))
        http_files  = sorted(dir_path.glob("http_login_*.json"))
        tcp_paths   = [str(p) for p in tcp_files]
        quic_paths  = [str(p) for p in quic_files]
        http_paths  = [str(p) for p in http_files]
        print(f"[compare_protocols] Found {len(tcp_paths)} TCP, {len(quic_paths)} QUIC, {len(http_paths)} HTTP reports in {args.input_dir}")
    else:
        tcp_paths  = expand(args.tcp_report)
        quic_paths = expand(args.quic_report)
        http_paths = expand(args.http_report)

    result = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "comparison": {},
    }

    if tcp_paths:
        result["comparison"]["tcp"] = aggregate_reports(tcp_paths)
    if quic_paths:
        result["comparison"]["quic"] = aggregate_reports(quic_paths)
    if http_paths:
        result["comparison"]["http"] = aggregate_reports(http_paths)

    # Compute speedup / efficiency deltas
    protocols = list(result["comparison"].keys())
    if len(protocols) >= 2:
        p0, p1 = protocols[0], protocols[1]
        r0 = result["comparison"][p0]
        r1 = result["comparison"][p1]

        def speedup(r_base: dict, r_cmp: dict) -> float:
            if r_base.get("latency_avg", 0) == 0:
                return 0.0
            return r_base["latency_avg"] / r_cmp.get("latency_avg", 0.001)

        def rps_improvement(r_base: dict, r_cmp: dict) -> float:
            if r_base.get("rps", 0) == 0:
                return 0.0
            return (r_cmp.get("rps", 0) - r_base["rps"]) / r_base["rps"] * 100.0

        result["comparison"]["_delta"] = {
            f"{p1}_vs_{p0}": {
                "latency_speedup": round(speedup(r0, r1), 2),
                "rps_improvement_pct": round(rps_improvement(r0, r1), 2),
                "success_rate_delta": round((r1.get("success_rate", 0) - r0.get("success_rate", 0)) * 100, 4),
            }
        }

    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)

    print(f"[compare_protocols] Report: {args.output}")
    print(json.dumps(result["comparison"], indent=2))


if __name__ == "__main__":
    main()
