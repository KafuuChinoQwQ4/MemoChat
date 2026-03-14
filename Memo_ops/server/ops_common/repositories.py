from __future__ import annotations

from .storage import fetch_all, fetch_one


def list_loadtest_runs(conn, limit: int) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT run_id, suite_name, scenario_name, status, summary_json, file_path,
               success_count, failure_count, started_at, finished_at
          FROM ops_loadtest_run
         ORDER BY COALESCE(finished_at, started_at) DESC
         LIMIT %s
        """,
        (limit,),
    )


def get_loadtest_run(conn, run_id: str) -> dict | None:
    return fetch_one(conn, "SELECT * FROM ops_loadtest_run WHERE run_id=%s", (run_id,))


def list_loadtest_cases(conn, run_id: str) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT case_name, status, success_count, failure_count, p50_ms, p95_ms, avg_ms, extras_json
          FROM ops_loadtest_case
         WHERE run_id=%s
         ORDER BY case_name
        """,
        (run_id,),
    )


def list_loadtest_error_buckets(conn, run_id: str) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT error_key, error_count, sample_message
          FROM ops_loadtest_error_bucket
         WHERE run_id=%s
         ORDER BY error_count DESC, error_key
        """,
        (run_id,),
    )


def get_trace_summary(conn, trace_id: str) -> dict | None:
    return fetch_one(conn, "SELECT * FROM ops_trace_index WHERE trace_id=%s", (trace_id,))


def list_trace_logs(conn, trace_id: str) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT ts_utc, level, service_name, service_instance, event_name, message, duration_ms
          FROM ops_log_event_index
         WHERE trace_id=%s
         ORDER BY ts_utc ASC, id ASC
        """,
        (trace_id,),
    )


def list_recent_service_snapshots(conn) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT service_name, instance_name, observed_at, cpu_percent, memory_rss_bytes, online_users,
               qps, error_rate, latency_p95_ms, port_up, status
          FROM ops_service_snapshot
         WHERE observed_at >= CURRENT_TIMESTAMP - INTERVAL '15 minutes'
         ORDER BY observed_at DESC, service_name, instance_name
        """
    )


def get_service_snapshot_history(conn, service_name: str) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT *
          FROM ops_service_snapshot
         WHERE service_name=%s
         ORDER BY observed_at DESC
         LIMIT 100
        """,
        (service_name,),
    )


def get_service_minute_series(conn, service_name: str) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT *
          FROM ops_service_metric_minute
         WHERE service_name=%s
         ORDER BY minute_utc DESC
         LIMIT 240
        """,
        (service_name,),
    )


def list_recent_alert_source_snapshots(conn) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT service_name, instance_name, cpu_percent, error_rate, status
          FROM ops_service_snapshot
         WHERE observed_at >= CURRENT_TIMESTAMP - INTERVAL '5 minutes'
         ORDER BY observed_at DESC
        """
    )


def list_latest_service_health(conn) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT s.service_name, s.instance_name, s.observed_at, s.cpu_percent, s.memory_rss_bytes,
               s.online_users, s.qps, s.error_rate, s.latency_p95_ms, s.port_up, s.status
          FROM ops_service_snapshot s
          JOIN (
                SELECT service_name, instance_name, MAX(observed_at) AS latest_observed_at
                  FROM ops_service_snapshot
                 GROUP BY service_name, instance_name
          ) latest
            ON latest.service_name = s.service_name
           AND latest.instance_name = s.instance_name
           AND latest.latest_observed_at = s.observed_at
         ORDER BY s.service_name, s.instance_name
        """
    )


def list_overview_recent_runs(conn, limit: int) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT run_id, suite_name, scenario_name, status, success_count, failure_count,
               started_at, finished_at, file_path
          FROM ops_loadtest_run
         ORDER BY COALESCE(finished_at, started_at) DESC
         LIMIT %s
        """,
        [limit],
    )


def summarize_alert_rows(conn) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT severity, COUNT(*) AS count_rows
          FROM (
            SELECT 'critical' AS severity, service_name, instance_name
              FROM ops_service_snapshot
             WHERE observed_at >= CURRENT_TIMESTAMP - INTERVAL '10 minutes'
               AND status <> 'up'
            UNION ALL
            SELECT 'warning' AS severity, service_name, instance_name
              FROM ops_service_snapshot
             WHERE observed_at >= CURRENT_TIMESTAMP - INTERVAL '10 minutes'
               AND (cpu_percent >= 85 OR error_rate >= 0.25)
          ) alerts
         GROUP BY severity
        """
    )


def list_top_error_services(conn, limit: int = 6) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT service_name,
               SUM(CASE WHEN level IN ('error','fatal','critical') THEN 1 ELSE 0 END) AS error_count
          FROM ops_log_event_index
         WHERE ts_utc >= CURRENT_TIMESTAMP - INTERVAL '1 hour'
         GROUP BY service_name
         ORDER BY error_count DESC, service_name
         LIMIT %s
        """,
        (limit,),
    )


def list_monitoring_overview_service_snapshots(conn) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT service_name, instance_name, observed_at, cpu_percent, memory_rss_bytes,
               online_users, qps, error_rate, latency_p95_ms, port_up, status
          FROM ops_service_snapshot
         WHERE observed_at >= CURRENT_TIMESTAMP - INTERVAL '10 minutes'
         ORDER BY observed_at DESC
        """
    )


def list_monitoring_overview_recent_log_counts(conn) -> list[dict]:
    return fetch_all(
        conn,
        """
        SELECT service_name, COUNT(*) AS count_rows
          FROM ops_log_event_index
         WHERE ts_utc >= CURRENT_TIMESTAMP - INTERVAL '60 minutes'
         GROUP BY service_name
        """
    )
