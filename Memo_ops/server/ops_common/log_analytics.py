from __future__ import annotations

from typing import Any

from .storage import fetch_all, fetch_one
from .time_windows import bucket_for_window, bucket_sql, normalize_window


def build_log_filters(
    service: str = "",
    instance: str = "",
    level: str = "",
    trace_id: str = "",
    request_id: str = "",
    event: str = "",
    keyword: str = "",
    from_utc: str | None = None,
    to_utc: str | None = None,
    default_hours: int = 24,
) -> tuple[str, list[Any], dict[str, Any], object, object]:
    start, end = normalize_window(from_utc, to_utc, default_hours)
    clauses = ["ts_utc >= %s", "ts_utc <= %s"]
    params: list[Any] = [start.strftime("%Y-%m-%d %H:%M:%S"), end.strftime("%Y-%m-%d %H:%M:%S")]
    applied = {
        "service": service,
        "instance": instance,
        "level": level,
        "trace_id": trace_id,
        "request_id": request_id,
        "event": event,
        "keyword": keyword,
        "from_utc": start.isoformat(),
        "to_utc": end.isoformat(),
    }
    if service:
        clauses.append("service_name = %s")
        params.append(service)
    if instance:
        clauses.append("service_instance = %s")
        params.append(instance)
    if level:
        clauses.append("level = %s")
        params.append(level)
    if trace_id:
        clauses.append("trace_id = %s")
        params.append(trace_id)
    if request_id:
        clauses.append("request_id = %s")
        params.append(request_id)
    if event:
        clauses.append("event_name = %s")
        params.append(event)
    if keyword:
        clauses.append("(message LIKE %s OR raw_json LIKE %s)")
        like = f"%{keyword}%"
        params.extend([like, like])
    return " WHERE " + " AND ".join(clauses), params, applied, start, end


def query_logs(
    conn,
    *,
    service: str = "",
    instance: str = "",
    level: str = "",
    trace_id: str = "",
    request_id: str = "",
    event: str = "",
    keyword: str = "",
    from_utc: str | None = None,
    to_utc: str | None = None,
    page: int = 1,
    page_size: int = 100,
    sort: str = "ts_desc",
) -> dict[str, Any]:
    page = max(page, 1)
    page_size = max(1, min(page_size, 500))
    order_by = "ts_utc DESC, id DESC" if sort != "ts_asc" else "ts_utc ASC, id ASC"
    where_sql, params, applied, _, _ = build_log_filters(
        service=service,
        instance=instance,
        level=level,
        trace_id=trace_id,
        request_id=request_id,
        event=event,
        keyword=keyword,
        from_utc=from_utc,
        to_utc=to_utc,
    )
    total_row = fetch_one(conn, f"SELECT COUNT(*) AS total FROM ops_log_event_index{where_sql}", params)
    total = int((total_row or {}).get("total", 0))
    offset = (page - 1) * page_size
    items = fetch_all(
        conn,
        f"""
        SELECT id, file_path, line_number, ts_utc, level, service_name, service_instance,
               trace_id, request_id, event_name, message, duration_ms, raw_json
          FROM ops_log_event_index
        {where_sql}
         ORDER BY {order_by}
         LIMIT %s OFFSET %s
        """,
        [*params, page_size, offset],
    )
    return {
        "items": items,
        "page": page,
        "page_size": page_size,
        "total": total,
        "has_more": (offset + len(items)) < total,
        "applied_filters": {**applied, "sort": sort},
    }


def query_log_trends(conn, **kwargs) -> dict[str, Any]:
    where_sql, params, applied, start, end = build_log_filters(
        service=kwargs.get("service", ""),
        instance=kwargs.get("instance", ""),
        level=kwargs.get("level", ""),
        trace_id=kwargs.get("trace_id", ""),
        request_id=kwargs.get("request_id", ""),
        event=kwargs.get("event", ""),
        keyword=kwargs.get("keyword", ""),
        from_utc=kwargs.get("from_utc"),
        to_utc=kwargs.get("to_utc"),
    )
    bucket_kind, bucket_label = bucket_for_window(start, end)
    bucket_expr = bucket_sql("ts_utc", bucket_kind)
    rows = fetch_all(
        conn,
        f"""
        SELECT {bucket_expr} AS bucket_utc,
               COUNT(*) AS total_count,
               SUM(CASE WHEN level IN ('error','fatal','critical') THEN 1 ELSE 0 END) AS error_count,
               SUM(CASE WHEN level IN ('warn','warning') THEN 1 ELSE 0 END) AS warn_count,
               AVG(duration_ms) AS avg_duration_ms
          FROM ops_log_event_index
        {where_sql}
         GROUP BY {bucket_expr}
         ORDER BY bucket_utc ASC
        """,
        params,
    )
    return {
        "bucket": bucket_label,
        "from_utc": start.isoformat(),
        "to_utc": end.isoformat(),
        "items": rows,
        "applied_filters": applied,
    }
