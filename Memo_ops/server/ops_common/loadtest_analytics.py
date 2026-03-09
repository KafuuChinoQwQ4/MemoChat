from __future__ import annotations

from typing import Any

from .storage import fetch_all
from .time_windows import normalize_window


def query_loadtest_trends(
    conn,
    from_utc: str | None = None,
    to_utc: str | None = None,
    group_by: str = "day",
) -> dict[str, Any]:
    start, end = normalize_window(from_utc, to_utc, 24 * 7)
    if group_by == "suite":
        bucket_expr = "suite_name"
        bucket_label = "suite"
    else:
        bucket_expr = "DATE_FORMAT(COALESCE(finished_at, started_at), '%%Y-%%m-%%d')"
        bucket_label = "day"
    rows = fetch_all(
        conn,
        f"""
        SELECT {bucket_expr} AS bucket,
               COUNT(*) AS run_count,
               SUM(CASE WHEN status='passed' THEN 1 ELSE 0 END) AS pass_count,
               SUM(CASE WHEN status='failed' THEN 1 ELSE 0 END) AS fail_count,
               AVG(
                   CASE
                       WHEN (success_count + failure_count) > 0
                       THEN success_count / (success_count + failure_count)
                       ELSE NULL
                   END
               ) AS avg_success_rate
          FROM ops_loadtest_run
         WHERE COALESCE(finished_at, started_at) >= %s
           AND COALESCE(finished_at, started_at) <= %s
         GROUP BY {bucket_expr}
         ORDER BY bucket ASC
        """,
        [start.strftime("%Y-%m-%d %H:%M:%S"), end.strftime("%Y-%m-%d %H:%M:%S")],
    )
    return {
        "bucket": bucket_label,
        "from_utc": start.isoformat(),
        "to_utc": end.isoformat(),
        "items": rows,
    }
