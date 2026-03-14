from __future__ import annotations

import hashlib
import json
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List

from Memo_ops.server.ops_common.config import resolve_path
from Memo_ops.server.ops_common.storage import execute, fetch_all, mysql_conn


def _json_dumps(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False)


def _iso_to_datetime(raw: str) -> datetime:
    if not raw:
        return datetime.utcnow()
    for fmt in ("%Y-%m-%dT%H:%M:%S.%fZ", "%Y-%m-%dT%H:%M:%SZ"):
        try:
            return datetime.strptime(raw, fmt)
        except ValueError:
            continue
    return datetime.utcnow()


def _file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        while True:
            chunk = fh.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def _iter_paths(config: Dict[str, Any], key: str) -> List[Path]:
    found: List[Path] = []
    config_root = Path(config["_config_dir"])
    for pattern in config["paths"].get(key, []):
        found.extend(sorted(config_root.glob(pattern)))
    return found


def import_reports(config: Dict[str, Any]) -> Dict[str, int]:
    imported = 0
    skipped = 0
    for path in _iter_paths(config, "report_globs"):
        if not path.is_file():
            continue
        try:
            payload = json.loads(path.read_text(encoding="utf-8-sig"))
        except Exception:
            skipped += 1
            continue
        if not isinstance(payload, dict):
            skipped += 1
            continue
        digest = _file_hash(path)
        run_id = hashlib.sha1(f"{path.as_posix()}:{digest}".encode("utf-8")).hexdigest()[:32]
        suite_name = path.parent.name
        with mysql_conn(config["postgresql"]) as conn:
            summary = payload.get("summary", {}) or {}
            started_at = _iso_to_datetime(str(payload.get("time_utc", "")))
            latency = summary.get("latency_ms", {}) or {}
            success_count = int(summary.get("success_count", summary.get("success", 0)) or 0)
            failure_count = int(summary.get("failure_count", summary.get("failed", 0)) or 0)
            execute(
                conn,
                """
                INSERT INTO ops_source(source_type, source_path, source_hash, metadata_json)
                VALUES(%s, %s, %s, %s::jsonb)
                ON CONFLICT (source_type, source_path) DO UPDATE
                SET source_hash = EXCLUDED.source_hash,
                    last_seen_at = CURRENT_TIMESTAMP,
                    metadata_json = EXCLUDED.metadata_json
                """,
                ("report", str(path), digest, _json_dumps({"suite_name": suite_name})),
            )
            execute(
                conn,
                """
                INSERT INTO ops_loadtest_run(
                    run_id, suite_name, scenario_name, test_name, file_path, report_hash, started_at, finished_at,
                    status, success_count, failure_count, summary_json, top_errors_json, phase_breakdown_json,
                    preconditions_json, data_mutation_json
                ) VALUES(%s, %s, %s, %s, %s, %s, %s, CURRENT_TIMESTAMP, %s, %s, %s, %s::jsonb, %s::jsonb,
                         %s::jsonb, %s::jsonb, %s::jsonb)
                ON CONFLICT (run_id) DO UPDATE
                SET suite_name = EXCLUDED.suite_name,
                    scenario_name = EXCLUDED.scenario_name,
                    test_name = EXCLUDED.test_name,
                    report_hash = EXCLUDED.report_hash,
                    started_at = EXCLUDED.started_at,
                    finished_at = EXCLUDED.finished_at,
                    status = EXCLUDED.status,
                    success_count = EXCLUDED.success_count,
                    failure_count = EXCLUDED.failure_count,
                    summary_json = EXCLUDED.summary_json,
                    top_errors_json = EXCLUDED.top_errors_json,
                    phase_breakdown_json = EXCLUDED.phase_breakdown_json,
                    preconditions_json = EXCLUDED.preconditions_json,
                    data_mutation_json = EXCLUDED.data_mutation_json
                """,
                (
                    run_id,
                    suite_name,
                    payload.get("scenario", ""),
                    payload.get("test", ""),
                    str(path),
                    digest,
                    started_at,
                    "failed" if failure_count > 0 else "passed",
                    success_count,
                    failure_count,
                    _json_dumps(summary),
                    _json_dumps(payload.get("top_errors", {})),
                    _json_dumps(payload.get("phase_breakdown", {})),
                    _json_dumps(payload.get("preconditions", {})),
                    _json_dumps(payload.get("data_mutation_summary", {})),
                ),
            )
            execute(conn, "DELETE FROM ops_loadtest_case WHERE run_id = %s", (run_id,))
            execute(conn, "DELETE FROM ops_loadtest_error_bucket WHERE run_id = %s", (run_id,))
            case_rows = payload.get("samples", []) or []
            if not case_rows:
                case_rows = [
                    {
                        "scenario": payload.get("test", path.stem),
                        "status": "failed" if failure_count > 0 else "passed",
                        "success_count": success_count,
                        "failure_count": failure_count,
                        "p50_ms": latency.get("p50", latency.get("avg", 0)),
                        "p95_ms": latency.get("p95", latency.get("p90", 0)),
                        "avg_ms": latency.get("avg", 0),
                        "target": payload.get("target", ""),
                    }
                ]
            for sample in case_rows:
                case_name = str(sample.get("scenario") or sample.get("email") or sample.get("group_id") or "sample")
                execute(
                    conn,
                    """
                    INSERT INTO ops_loadtest_case(
                        run_id, case_name, status, success_count, failure_count, p50_ms, p95_ms, avg_ms, extras_json
                    ) VALUES(%s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb)
                    ON CONFLICT (run_id, case_name) DO UPDATE
                    SET status=EXCLUDED.status,
                        success_count=EXCLUDED.success_count,
                        failure_count=EXCLUDED.failure_count,
                        p50_ms=EXCLUDED.p50_ms,
                        p95_ms=EXCLUDED.p95_ms,
                        avg_ms=EXCLUDED.avg_ms,
                        extras_json=EXCLUDED.extras_json
                    """,
                    (
                        run_id,
                        case_name,
                        str(sample.get("status", "sample")),
                        int(sample.get("success_count", 0) or 0),
                        int(sample.get("failure_count", 0) or 0),
                        float(sample.get("p50_ms", 0) or 0),
                        float(sample.get("p95_ms", 0) or 0),
                        float(sample.get("avg_ms", 0) or 0),
                        _json_dumps(sample),
                    ),
                )
            for error_key, count in (payload.get("error_counter", {}) or {}).items():
                execute(
                    conn,
                    """
                    INSERT INTO ops_loadtest_error_bucket(run_id, error_key, error_count, sample_message)
                    VALUES(%s, %s, %s, %s)
                    ON CONFLICT (run_id, error_key) DO UPDATE
                    SET error_count=EXCLUDED.error_count, sample_message=EXCLUDED.sample_message
                    """,
                    (run_id, error_key, int(count), str(error_key)),
                )
        imported += 1
    return {"imported": imported, "skipped": skipped}


def import_logs(config: Dict[str, Any]) -> Dict[str, int]:
    imported = 0
    skipped = 0
    for path in _iter_paths(config, "log_globs"):
        if not path.is_file():
            continue
        line_number = 0
        with mysql_conn(config["postgresql"]) as conn:
            for line_number, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1):
                if not raw.strip():
                    continue
                try:
                    payload = json.loads(raw)
                except Exception:
                    skipped += 1
                    continue
                ts = _iso_to_datetime(str(payload.get("ts", "")))
                execute(
                    conn,
                    """
                    INSERT INTO ops_log_stream(service_name, service_instance, source_path, source_type)
                    VALUES(%s, %s, %s, 'json_log')
                    ON CONFLICT (source_path) DO UPDATE
                    SET service_name = EXCLUDED.service_name,
                        service_instance = EXCLUDED.service_instance,
                        updated_at = CURRENT_TIMESTAMP
                    """,
                    (str(payload.get("service", "")), str(payload.get("service_instance", "")), str(path)),
                )
                execute(
                    conn,
                    """
                    INSERT INTO ops_log_event_index(
                        service_name, service_instance, level, event_name, trace_id, request_id, uid, ts_utc,
                        duration_ms, file_path, line_number, message, attrs_json, raw_json
                    ) VALUES(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb, %s)
                    ON CONFLICT (file_path, line_number) DO UPDATE
                    SET service_name = EXCLUDED.service_name,
                        service_instance = EXCLUDED.service_instance,
                        level = EXCLUDED.level,
                        event_name = EXCLUDED.event_name,
                        trace_id = EXCLUDED.trace_id,
                        request_id = EXCLUDED.request_id,
                        uid = EXCLUDED.uid,
                        ts_utc = EXCLUDED.ts_utc,
                        duration_ms = EXCLUDED.duration_ms,
                        message = EXCLUDED.message,
                        attrs_json = EXCLUDED.attrs_json,
                        raw_json = EXCLUDED.raw_json
                    """,
                    (
                        str(payload.get("service", "")),
                        str(payload.get("service_instance", "")),
                        str(payload.get("level", "")),
                        str(payload.get("event", "")),
                        str(payload.get("trace_id", "")),
                        str(payload.get("request_id", "")),
                        str(payload.get("uid", "")),
                        ts,
                        float(payload.get("duration_ms", 0) or 0),
                        str(path),
                        line_number,
                        str(payload.get("message", ""))[:1024],
                        _json_dumps(payload.get("attrs", {})),
                        json.dumps(payload, ensure_ascii=False),
                    ),
                )
            execute(
                conn,
                """
                INSERT INTO ops_source(source_type, source_path, source_hash, metadata_json)
                VALUES(%s, %s, %s, %s::jsonb)
                ON CONFLICT (source_type, source_path) DO UPDATE
                SET source_hash = EXCLUDED.source_hash,
                    last_seen_at = CURRENT_TIMESTAMP
                """,
                ("log", str(path), _file_hash(path), _json_dumps({"lines_indexed": line_number})),
            )
        imported += 1
    rebuild_trace_index(config)
    return {"imported": imported, "skipped": skipped}


def rebuild_trace_index(config: Dict[str, Any]) -> None:
    with mysql_conn(config["postgresql"]) as conn:
        execute(conn, "DELETE FROM ops_trace_index")
        rows = fetch_all(
            conn,
            """
            SELECT trace_id,
                   MIN(ts_utc) AS first_seen_at,
                   MAX(ts_utc) AS last_seen_at,
                   COUNT(*) AS log_count,
                   (array_agg(service_name ORDER BY ts_utc ASC))[1] AS root_service
            FROM ops_log_event_index
            WHERE trace_id <> ''
            GROUP BY trace_id
            """,
        )
        for row in rows:
            services = fetch_all(
                conn,
                "SELECT DISTINCT service_name FROM ops_log_event_index WHERE trace_id = %s ORDER BY service_name",
                (row["trace_id"],),
            )
            execute(
                conn,
                """
                INSERT INTO ops_trace_index(trace_id, first_seen_at, last_seen_at, services_json, log_count, span_count, root_service, status)
                VALUES(%s, %s, %s, %s::jsonb, %s, %s, %s, 'indexed')
                """,
                (
                    row["trace_id"],
                    row["first_seen_at"],
                    row["last_seen_at"],
                    _json_dumps([service["service_name"] for service in services]),
                    int(row["log_count"]),
                    0,
                    row.get("root_service", "") or "",
                ),
            )
