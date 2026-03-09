from __future__ import annotations

from datetime import datetime, timedelta, timezone


def utc_now() -> datetime:
    return datetime.now(timezone.utc)


def parse_utc(raw: str | None, default: datetime) -> datetime:
    if not raw:
        return default
    value = raw.strip()
    if value.endswith("Z"):
        value = value[:-1] + "+00:00"
    try:
        parsed = datetime.fromisoformat(value)
    except ValueError:
        return default
    if parsed.tzinfo is None:
        return parsed.replace(tzinfo=timezone.utc)
    return parsed.astimezone(timezone.utc)


def normalize_window(from_utc: str | None, to_utc: str | None, default_hours: int) -> tuple[datetime, datetime]:
    end = parse_utc(to_utc, utc_now())
    start = parse_utc(from_utc, end - timedelta(hours=default_hours))
    if start > end:
        start, end = end, start
    return start, end


def bucket_for_window(start: datetime, end: datetime) -> tuple[str, str]:
    hours = max((end - start).total_seconds() / 3600.0, 0.01)
    if hours <= 6:
        return "5m", "5m"
    if hours <= 24:
        return "15m", "15m"
    if hours <= 24 * 7:
        return "1h", "1h"
    return "1d", "1d"


def bucket_sql(column: str, bucket: str) -> str:
    if bucket == "5m":
        return f"DATE_FORMAT(DATE_SUB({column}, INTERVAL (MINUTE({column}) %% 5) MINUTE), '%%Y-%%m-%%dT%%H:%%i:00')"
    if bucket == "15m":
        return f"DATE_FORMAT(DATE_SUB({column}, INTERVAL (MINUTE({column}) %% 15) MINUTE), '%%Y-%%m-%%dT%%H:%%i:00')"
    if bucket == "1h":
        return f"DATE_FORMAT({column}, '%%Y-%%m-%%dT%%H:00:00')"
    return f"DATE_FORMAT({column}, '%%Y-%%m-%%dT00:00:00')"
