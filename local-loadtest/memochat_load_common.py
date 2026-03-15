import base64
import csv
import datetime as _dt
import hashlib
import json
import logging
import math
import os
import random
import socket
import string
import struct
import tempfile
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from logging.handlers import TimedRotatingFileHandler
from typing import Any, Callable, Dict, Iterable, List, Optional, Sequence, Tuple


ID_GET_VARIFY_CODE = 1001
ID_REG_USER = 1002
ID_RESET_PWD = 1003
ID_LOGIN_USER = 1004
ID_CHAT_LOGIN = 1005
ID_CHAT_LOGIN_RSP = 1006
ID_SEARCH_USER_REQ = 1007
ID_SEARCH_USER_RSP = 1008
ID_ADD_FRIEND_REQ = 1009
ID_ADD_FRIEND_RSP = 1010
ID_NOTIFY_ADD_FRIEND_REQ = 1011
ID_AUTH_FRIEND_REQ = 1013
ID_AUTH_FRIEND_RSP = 1014
ID_NOTIFY_AUTH_FRIEND_REQ = 1015
ID_TEXT_CHAT_MSG_REQ = 1017
ID_TEXT_CHAT_MSG_RSP = 1018
ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019
ID_NOTIFY_OFF_LINE_REQ = 1021
ID_HEART_BEAT_REQ = 1023
ID_HEARTBEAT_RSP = 1024
ID_CREATE_GROUP_REQ = 1031
ID_CREATE_GROUP_RSP = 1032
ID_GET_GROUP_LIST_REQ = 1033
ID_GET_GROUP_LIST_RSP = 1034
ID_INVITE_GROUP_MEMBER_REQ = 1035
ID_INVITE_GROUP_MEMBER_RSP = 1036
ID_NOTIFY_GROUP_INVITE_REQ = 1037
ID_APPLY_JOIN_GROUP_REQ = 1038
ID_APPLY_JOIN_GROUP_RSP = 1039
ID_NOTIFY_GROUP_APPLY_REQ = 1040
ID_REVIEW_GROUP_APPLY_REQ = 1041
ID_REVIEW_GROUP_APPLY_RSP = 1042
ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ = 1043
ID_GROUP_CHAT_MSG_REQ = 1044
ID_GROUP_CHAT_MSG_RSP = 1045
ID_NOTIFY_GROUP_CHAT_MSG_REQ = 1046
ID_GROUP_HISTORY_REQ = 1047
ID_GROUP_HISTORY_RSP = 1048
ID_PRIVATE_HISTORY_REQ = 1059
ID_PRIVATE_HISTORY_RSP = 1060
ID_GROUP_READ_ACK_REQ = 1071
ID_PRIVATE_READ_ACK_REQ = 1076
ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ = 1081
ID_NOTIFY_PRIVATE_READ_ACK_REQ = 1082
ID_NOTIFY_MSG_STATUS_REQ = 1094


def utc_now_str() -> str:
    return _dt.datetime.utcnow().strftime("%Y%m%d_%H%M%S")


def now_ms() -> int:
    return int(time.time() * 1000)


def ensure_dir(path: str) -> None:
    if path:
        os.makedirs(path, exist_ok=True)


def load_json(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8-sig") as f:
        payload = json.load(f)
    if isinstance(payload, dict):
        payload["__config_dir"] = os.path.dirname(os.path.abspath(path))
    return payload


def save_json(path: str, payload: Dict[str, Any]) -> None:
    ensure_dir(os.path.dirname(path))
    with open(path, "w", encoding="utf-8-sig") as f:
        json.dump(payload, f, ensure_ascii=False, indent=2)


def xor_encode(raw: str) -> str:
    x = len(raw) % 255
    return "".join(chr(ord(ch) ^ x) for ch in raw)


def pct(values: List[float], p: float) -> float:
    if not values:
        return 0.0
    sorted_vals = sorted(values)
    idx = int(math.ceil(len(sorted_vals) * p)) - 1
    idx = max(0, min(idx, len(sorted_vals) - 1))
    return float(sorted_vals[idx])


def build_summary(latencies_ms: List[float], success: int, failed: int, elapsed_sec: float) -> Dict[str, Any]:
    total = success + failed
    return {
        "total": total,
        "success": success,
        "failed": failed,
        "success_rate": round((success / total) if total else 0.0, 6),
        "elapsed_sec": round(elapsed_sec, 6),
        "throughput_rps": round((total / elapsed_sec) if elapsed_sec > 0 else 0.0, 3),
        "latency_ms": {
            "min": round(min(latencies_ms), 3) if latencies_ms else 0.0,
            "avg": round((sum(latencies_ms) / len(latencies_ms)), 3) if latencies_ms else 0.0,
            "p50": round(pct(latencies_ms, 0.50), 3) if latencies_ms else 0.0,
            "p90": round(pct(latencies_ms, 0.90), 3) if latencies_ms else 0.0,
            "p95": round(pct(latencies_ms, 0.95), 3) if latencies_ms else 0.0,
            "p99": round(pct(latencies_ms, 0.99), 3) if latencies_ms else 0.0,
            "max": round(max(latencies_ms), 3) if latencies_ms else 0.0,
        },
    }


class JsonFormatter(logging.Formatter):
    def format(self, record: logging.LogRecord) -> str:
        attrs: Dict[str, Any] = {}
        payload = {
            "ts": _dt.datetime.utcnow().isoformat(timespec="milliseconds") + "Z",
            "level": record.levelname.lower(),
            "service": getattr(record, "service", record.name),
            "service_instance": getattr(record, "service_instance", f"{socket.gethostname()}:{os.getpid()}"),
            "event": getattr(record, "event", record.name),
            "message": record.getMessage(),
            "thread": threading.current_thread().name,
            "request_id": getattr(record, "request_id", ""),
            "span_id": getattr(record, "span_id", ""),
            "uid": getattr(record, "uid", ""),
            "session_id": getattr(record, "session_id", ""),
            "module": getattr(record, "module", ""),
            "peer_service": getattr(record, "peer_service", ""),
            "error_code": getattr(record, "error_code", ""),
            "error_type": getattr(record, "error_type", ""),
            "duration_ms": getattr(record, "duration_ms", 0),
        }
        for key in ("trace_id", "scenario", "stage", "account_email", "group_id", "peer_uid", "upload_id"):
            value = getattr(record, key, "")
            if value not in ("", None):
                if key == "trace_id":
                    payload[key] = value
                else:
                    attrs[key] = value
        extra_payload = getattr(record, "payload", None)
        if isinstance(extra_payload, dict):
            attrs.update(extra_payload)
        payload["attrs"] = attrs
        return json.dumps(payload, ensure_ascii=False)


def init_json_logger(
    name: str,
    log_dir: str = "logs",
    to_console: bool = True,
    level: str = "INFO",
) -> logging.Logger:
    log_dir = os.environ.get("MEMOCHAT_LOADTEST_LOG_DIR", log_dir)
    ensure_dir(log_dir)
    logger = logging.getLogger(name)
    logger.setLevel(getattr(logging, level.upper(), logging.INFO))
    logger.propagate = False
    if logger.handlers:
        return logger

    formatter = JsonFormatter()

    file_handler = TimedRotatingFileHandler(
        filename=os.path.join(log_dir, f"{name}.json"),
        when="midnight",
        backupCount=14,
        encoding="utf-8",
        utc=True,
    )
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)

    if to_console:
        stream_handler = logging.StreamHandler()
        stream_handler.setFormatter(formatter)
        logger.addHandler(stream_handler)

    return logger


def new_trace_id() -> str:
    return uuid.uuid4().hex


def new_request_id() -> str:
    return uuid.uuid4().hex


def new_span_id() -> str:
    return uuid.uuid4().hex[:16]


def telemetry_config(cfg: Dict[str, Any]) -> Dict[str, Any]:
    telemetry = dict(cfg.get("telemetry", {}))
    return {
        "enabled": bool(telemetry.get("enabled", True)),
        "endpoint": str(telemetry.get("otlp_endpoint", "http://127.0.0.1:9411/api/v2/spans")).strip(),
        "export_traces": bool(telemetry.get("export_traces", True)),
        "service_name": str(telemetry.get("service_name", "memochat-loadtest")).strip() or "memochat-loadtest",
        "service_namespace": str(telemetry.get("service_namespace", "memochat")).strip() or "memochat",
    }


def export_zipkin_span(
    cfg: Dict[str, Any],
    name: str,
    kind: str,
    trace_id: str,
    span_id: str,
    parent_span_id: str = "",
    start_ms: int = 0,
    duration_ms: float = 0.0,
    attrs: Optional[Dict[str, Any]] = None,
) -> None:
    telemetry = telemetry_config(cfg)
    if not telemetry["enabled"] or not telemetry["export_traces"] or not telemetry["endpoint"] or not trace_id or not span_id:
        return
    payload = [
        {
            "traceId": trace_id,
            "id": span_id,
            "parentId": parent_span_id or None,
            "name": name,
            "kind": kind,
            "timestamp": str(int(start_ms) * 1000),
            "duration": str(int(max(duration_ms, 0.0) * 1000)),
            "localEndpoint": {"serviceName": telemetry["service_name"]},
            "tags": {
                "service.instance.id": f"{socket.gethostname()}:{os.getpid()}",
                "service.namespace": telemetry["service_namespace"],
                **{str(k): str(v) for k, v in (attrs or {}).items() if v not in ("", None)},
            },
        }
    ]
    data = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    req = urllib.request.Request(
        telemetry["endpoint"],
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=1.0):
            pass
    except Exception:
        return


def gate_url(cfg: Dict[str, Any]) -> str:
    return str(cfg.get("gate_url", "http://127.0.0.1:8080")).rstrip("/")


def gate_api_url(cfg: Dict[str, Any], path: str) -> str:
    return f"{gate_url(cfg)}{path}"


def login_path(cfg: Dict[str, Any]) -> str:
    return str(cfg.get("login_path", "/user_login"))


def client_ver(cfg: Dict[str, Any]) -> str:
    return str(cfg.get("client_ver", "2.0.0"))


def use_xor_passwd(cfg: Dict[str, Any]) -> bool:
    return bool(cfg.get("use_xor_passwd", True))


def get_runtime_accounts_csv(cfg: Dict[str, Any], cli_csv_path: str = "") -> str:
    if cli_csv_path:
        return cli_csv_path
    runtime_csv = str(cfg.get("runtime_accounts_csv", "accounts.local.csv")).strip()
    return resolve_cfg_path(cfg, runtime_csv)


def resolve_cfg_path(cfg: Dict[str, Any], value: str) -> str:
    raw = str(value or "").strip()
    if not raw:
        return raw
    if os.path.isabs(raw):
        return raw
    config_dir = str(cfg.get("__config_dir", "")).strip()
    if config_dir:
        return os.path.normpath(os.path.join(config_dir, raw))
    return raw


def get_log_dir(cfg: Dict[str, Any]) -> str:
    env_dir = os.environ.get("MEMOCHAT_LOADTEST_LOG_DIR", "").strip()
    if env_dir:
        return env_dir
    configured = str(cfg.get("log_dir", "logs")).strip()
    return resolve_cfg_path(cfg, configured)


def get_report_dir(cfg: Dict[str, Any]) -> str:
    env_dir = os.environ.get("MEMOCHAT_LOADTEST_REPORT_DIR", "").strip()
    if env_dir:
        return env_dir
    configured = str(cfg.get("report_dir", "reports")).strip()
    return resolve_cfg_path(cfg, configured)


def normalize_account(row: Dict[str, Any]) -> Dict[str, str]:
    normalized: Dict[str, str] = {}
    for key, value in row.items():
        if key is None:
            continue
        normalized[str(key).strip()] = "" if value is None else str(value).strip()
    return normalized


def load_accounts(config: Dict[str, Any], csv_path: str = "") -> List[Dict[str, str]]:
    accounts: List[Dict[str, str]] = []
    if csv_path:
        if not os.path.exists(csv_path):
            return accounts
        with open(csv_path, "r", encoding="utf-8-sig", newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                normalized = normalize_account(row)
                email = normalized.get("email", "")
                password = normalized.get("password", "") or normalized.get("last_password", "")
                if email and password:
                    normalized["password"] = password
                    accounts.append(normalized)
        return accounts

    for it in config.get("accounts", []):
        normalized = normalize_account(it)
        email = normalized.get("email", "")
        password = normalized.get("password", "") or normalized.get("last_password", "")
        if email and password:
            normalized["password"] = password
            accounts.append(normalized)
    return accounts


def filter_login_ready_accounts(
    cfg: Dict[str, Any],
    accounts: List[Dict[str, str]],
    timeout: float,
    logger: Optional[logging.Logger] = None,
) -> List[Dict[str, str]]:
    ready: List[Dict[str, str]] = []
    for account in accounts:
        email = str(account.get("email", "")).strip()
        password = str(account.get("password", "")).strip()
        if not email or not password:
            continue
        trace_id = new_trace_id()
        try:
            status, payload = gate_login(cfg, email, password, timeout, trace_id)
        except Exception as exc:  # noqa: BLE001
            if logger is not None:
                logger.warning(
                    "account login probe failed",
                    extra={
                        "event": "loadtest.accounts.login_probe_failed",
                        "trace_id": trace_id,
                        "account_email": email,
                        "payload": {"error": str(exc)},
                    },
                )
            continue

        if status != 200 or int(payload.get("error", -1)) != 0:
            if logger is not None:
                logger.warning(
                    "account rejected by login probe",
                    extra={
                        "event": "loadtest.accounts.login_probe_rejected",
                        "trace_id": trace_id,
                        "account_email": email,
                        "payload": {"http_status": status, "response_error": payload.get("error", -1)},
                    },
                )
            continue

        enriched = dict(account)
        enriched["uid"] = str(payload.get("uid", enriched.get("uid", "")))
        enriched["user_id"] = str(payload.get("user_id", enriched.get("user_id", "")))
        ready.append(enriched)
    return ready


def save_accounts_csv(path: str, accounts: List[Dict[str, Any]]) -> None:
    ensure_dir(os.path.dirname(path))
    fieldnames: List[str] = []
    seen = set()
    for key in ("email", "password", "user", "uid", "user_id", "last_password", "tags"):
        seen.add(key)
        fieldnames.append(key)
    for account in accounts:
        for key in account.keys():
            if key not in seen:
                seen.add(key)
                fieldnames.append(key)

    with open(path, "w", encoding="utf-8-sig", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for account in accounts:
            row = {field: account.get(field, "") for field in fieldnames}
            writer.writerow(row)


def upsert_accounts(existing: List[Dict[str, str]], updates: List[Dict[str, str]]) -> List[Dict[str, str]]:
    merged = {row.get("email", ""): dict(row) for row in existing if row.get("email")}
    for update in updates:
        email = update.get("email", "")
        if not email:
            continue
        if email not in merged:
            merged[email] = dict(update)
            continue
        merged[email].update({k: v for k, v in update.items() if v not in ("", None)})
    return [merged[email] for email in sorted(merged.keys())]


def config_path_relative(base_dir: str, path: str) -> str:
    if os.path.isabs(path):
        return path
    return os.path.join(base_dir, path)


def random_string(size: int, alphabet: str = string.ascii_lowercase + string.digits) -> str:
    return "".join(random.choice(alphabet) for _ in range(size))


def random_password() -> str:
    return "Pwd" + random_string(9, string.ascii_letters + string.digits)


def generate_account(seed_prefix: str, domain: str) -> Dict[str, str]:
    compact_prefix = "".join(ch for ch in seed_prefix.lower() if ch.isalnum())[:8] or "account"
    millis = f"{int(time.time() * 1000) % 100_000_000:08d}"
    user = f"{compact_prefix}_{millis}{random_string(7)}"
    return {
        "user": user[:24],
        "email": f"{user[:24]}@{domain}",
        "password": random_password(),
        "last_password": random_password(),
        "tags": "loadtest",
    }


def sha256_file(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def ensure_temp_file(size_bytes: int, suffix: str = ".bin") -> str:
    fd, path = tempfile.mkstemp(prefix="memochat-load-", suffix=suffix)
    os.close(fd)
    chunk = (b"MemoChatLoadTest-" * 8192)[:1024 * 1024]
    remaining = size_bytes
    with open(path, "wb") as f:
        while remaining > 0:
            piece = chunk[: min(len(chunk), remaining)]
            f.write(piece)
            remaining -= len(piece)
    return path


def get_mysql_config(cfg: Dict[str, Any]) -> Dict[str, Any]:
    mysql_cfg = dict(cfg.get("mysql", {}))
    return {
        "host": str(mysql_cfg.get("host", "127.0.0.1")),
        "port": int(mysql_cfg.get("port", 3306)),
        "user": str(mysql_cfg.get("user", "root")),
        "password": str(mysql_cfg.get("password", "123456")),
        "database": str(mysql_cfg.get("database", "memo")),
        "charset": str(mysql_cfg.get("charset", "utf8mb4")),
        "connect_timeout": int(mysql_cfg.get("connect_timeout_sec", 5)),
        "autocommit": bool(mysql_cfg.get("autocommit", True)),
    }


def get_postgresql_config(cfg: Dict[str, Any]) -> Dict[str, Any]:
    pg_cfg = dict(cfg.get("postgresql", {}))
    return {
        "host": str(pg_cfg.get("host", "127.0.0.1")),
        "port": int(pg_cfg.get("port", 5432)),
        "user": str(pg_cfg.get("user", "memochat")),
        "password": str(pg_cfg.get("password", "123456")),
        "database": str(pg_cfg.get("database", "memo_pg")),
        "schema": str(pg_cfg.get("schema", "public")),
        "sslmode": str(pg_cfg.get("sslmode", "disable")),
        "connect_timeout": int(pg_cfg.get("connect_timeout_sec", 5)),
        "autocommit": bool(pg_cfg.get("autocommit", True)),
    }


def get_redis_config(cfg: Dict[str, Any]) -> Dict[str, Any]:
    redis_cfg = dict(cfg.get("redis", {}))
    return {
        "host": str(redis_cfg.get("host", "127.0.0.1")),
        "port": int(redis_cfg.get("port", 6379)),
        "password": str(redis_cfg.get("password", "123456")),
        "db": int(redis_cfg.get("db", 0)),
        "socket_timeout": float(redis_cfg.get("timeout_sec", 5)),
        "decode_responses": bool(redis_cfg.get("decode_responses", True)),
    }


def open_mysql(cfg: Dict[str, Any]):
    if cfg.get("postgresql"):
        return open_postgresql(cfg)

    import pymysql

    mysql_cfg = get_mysql_config(cfg)
    return pymysql.connect(
        host=mysql_cfg["host"],
        port=mysql_cfg["port"],
        user=mysql_cfg["user"],
        password=mysql_cfg["password"],
        database=mysql_cfg["database"],
        charset=mysql_cfg["charset"],
        connect_timeout=mysql_cfg["connect_timeout"],
        autocommit=mysql_cfg["autocommit"],
        cursorclass=pymysql.cursors.DictCursor,
    )


def open_postgresql(cfg: Dict[str, Any]):
    import psycopg
    from psycopg.rows import dict_row

    pg_cfg = get_postgresql_config(cfg)
    return psycopg.connect(
        host=pg_cfg["host"],
        port=pg_cfg["port"],
        user=pg_cfg["user"],
        password=pg_cfg["password"],
        dbname=pg_cfg["database"],
        sslmode=pg_cfg["sslmode"],
        connect_timeout=pg_cfg["connect_timeout"],
        options=f"-c search_path={pg_cfg['schema']},public",
        autocommit=pg_cfg["autocommit"],
        row_factory=dict_row,
    )


def open_redis(cfg: Dict[str, Any]):
    import redis

    redis_cfg = get_redis_config(cfg)
    return redis.Redis(
        host=redis_cfg["host"],
        port=redis_cfg["port"],
        password=redis_cfg["password"],
        db=redis_cfg["db"],
        socket_timeout=redis_cfg["socket_timeout"],
        decode_responses=redis_cfg["decode_responses"],
    )


def http_json_request(
    url: str,
    payload: Optional[Dict[str, Any]] = None,
    headers: Optional[Dict[str, str]] = None,
    timeout: float = 5.0,
    method: str = "POST",
) -> Tuple[int, Dict[str, Any]]:
    request_headers = dict(headers or {})
    data = None
    if payload is not None:
        request_headers.setdefault("Content-Type", "application/json")
        data = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers=request_headers, method=method)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        status = int(getattr(resp, "status", 0) or 0)
        raw = resp.read().decode("utf-8", errors="replace")
    parsed = json.loads(raw) if raw else {}
    if not isinstance(parsed, dict):
        raise ValueError("response is not a JSON object")
    return status, parsed


def http_binary_request(
    url: str,
    body: bytes,
    headers: Optional[Dict[str, str]] = None,
    timeout: float = 30.0,
) -> Tuple[int, Dict[str, Any]]:
    request_headers = dict(headers or {})
    request_headers.setdefault("Content-Type", "application/octet-stream")
    req = urllib.request.Request(url, data=body, headers=request_headers, method="POST")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        status = int(getattr(resp, "status", 0) or 0)
        raw = resp.read().decode("utf-8", errors="replace")
    parsed = json.loads(raw) if raw else {}
    if not isinstance(parsed, dict):
        raise ValueError("response is not a JSON object")
    return status, parsed


def http_download(url: str, timeout: float = 30.0) -> Tuple[int, bytes, Dict[str, str]]:
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        status = int(getattr(resp, "status", 0) or 0)
        body = resp.read()
        headers = dict(resp.headers.items())
    return status, body, headers


def gate_login(cfg: Dict[str, Any], email: str, password: str, timeout: float, trace_id: str) -> Tuple[int, Dict[str, Any]]:
    request_id = new_request_id()
    span_id = new_span_id()
    passwd = xor_encode(password) if use_xor_passwd(cfg) else password
    body = {
        "email": email,
        "passwd": passwd,
        "client_ver": client_ver(cfg),
    }
    started = now_ms()
    try:
        return http_json_request(
            gate_api_url(cfg, login_path(cfg)),
            payload=body,
            headers={"X-Trace-Id": trace_id, "X-Request-Id": request_id, "X-Span-Id": span_id},
            timeout=timeout,
            method="POST",
        )
    finally:
        export_zipkin_span(cfg, f"HTTP POST {login_path(cfg)}", "CLIENT", trace_id, span_id, "", started, now_ms() - started, {"http.url": gate_api_url(cfg, login_path(cfg))})


def request_verify_code(cfg: Dict[str, Any], email: str, timeout: float, trace_id: str) -> Tuple[int, Dict[str, Any]]:
    request_id = new_request_id()
    span_id = new_span_id()
    started = now_ms()
    try:
        return http_json_request(
            gate_api_url(cfg, "/get_varifycode"),
            payload={"email": email},
            headers={"X-Trace-Id": trace_id, "X-Request-Id": request_id, "X-Span-Id": span_id},
            timeout=timeout,
            method="POST",
        )
    finally:
        export_zipkin_span(cfg, "HTTP POST /get_varifycode", "CLIENT", trace_id, span_id, "", started, now_ms() - started, {"http.url": gate_api_url(cfg, "/get_varifycode")})


def fetch_verify_code_from_redis(cfg: Dict[str, Any], email: str, wait_sec: float = 5.0) -> str:
    redis_conn = open_redis(cfg)
    key = f"code_{email}"
    deadline = time.time() + wait_sec
    try:
        while time.time() < deadline:
            value = redis_conn.get(key)
            if value:
                return str(value)
            time.sleep(0.1)
    finally:
        try:
            redis_conn.close()
        except Exception:
            pass
    raise TimeoutError(f"verify code not found in redis for {email}")


def register_via_gate(
    cfg: Dict[str, Any],
    user: str,
    email: str,
    password: str,
    verify_code: str,
    timeout: float,
    trace_id: str,
) -> Tuple[int, Dict[str, Any]]:
    request_id = new_request_id()
    span_id = new_span_id()
    encoded = xor_encode(password) if use_xor_passwd(cfg) else password
    body = {
        "user": user,
        "email": email,
        "passwd": encoded,
        "confirm": encoded,
        "varifycode": verify_code,
        "sex": 0,
        "icon": ":/res/head_1.jpg",
        "nick": user,
    }
    started = now_ms()
    try:
        return http_json_request(
            gate_api_url(cfg, "/user_register"),
            payload=body,
            headers={"X-Trace-Id": trace_id, "X-Request-Id": request_id, "X-Span-Id": span_id},
            timeout=timeout,
            method="POST",
        )
    finally:
        export_zipkin_span(cfg, "HTTP POST /user_register", "CLIENT", trace_id, span_id, "", started, now_ms() - started, {"http.url": gate_api_url(cfg, "/user_register")})


def reset_password_via_gate(
    cfg: Dict[str, Any],
    user: str,
    email: str,
    password: str,
    verify_code: str,
    timeout: float,
    trace_id: str,
) -> Tuple[int, Dict[str, Any]]:
    request_id = new_request_id()
    span_id = new_span_id()
    encoded = xor_encode(password) if use_xor_passwd(cfg) else password
    body = {
        "name": user,
        "user": user,
        "email": email,
        "passwd": encoded,
        "varifycode": verify_code,
    }
    started = now_ms()
    try:
        return http_json_request(
            gate_api_url(cfg, "/reset_pwd"),
            payload=body,
            headers={"X-Trace-Id": trace_id, "X-Request-Id": request_id, "X-Span-Id": span_id},
            timeout=timeout,
            method="POST",
        )
    finally:
        export_zipkin_span(cfg, "HTTP POST /reset_pwd", "CLIENT", trace_id, span_id, "", started, now_ms() - started, {"http.url": gate_api_url(cfg, "/reset_pwd")})


def normalize_host(host: str) -> str:
    host = (host or "").strip()
    if host in ("", "0.0.0.0", "::", "::1"):
        return "127.0.0.1"
    return host


def recv_exact(sock: socket.socket, size: int) -> bytes:
    out = bytearray()
    while len(out) < size:
        chunk = sock.recv(size - len(out))
        if not chunk:
            raise ConnectionError("socket closed while reading")
        out.extend(chunk)
    return bytes(out)


class TcpChatClient:
    def __init__(self, label: str, host: str, port: int, timeout: float):
        self.label = label
        self.host = normalize_host(host)
        self.port = int(port)
        self.timeout = timeout
        self.sock: Optional[socket.socket] = None

    def connect(self) -> None:
        self.sock = socket.create_connection((self.host, self.port), timeout=self.timeout)
        self.sock.settimeout(self.timeout)

    def close(self) -> None:
        if self.sock is None:
            return
        try:
            self.sock.close()
        finally:
            self.sock = None

    def send_frame(self, msg_id: int, body: bytes) -> None:
        if self.sock is None:
            raise RuntimeError("socket is not connected")
        header = struct.pack("!HH", int(msg_id), len(body))
        self.sock.sendall(header + body)

    def send_json(self, msg_id: int, payload: Dict[str, Any]) -> None:
        trace_payload = dict(payload)
        trace_payload.setdefault("trace_id", new_trace_id())
        trace_payload.setdefault("request_id", new_request_id())
        trace_payload.setdefault("span_id", new_span_id())
        body = json.dumps(trace_payload, separators=(",", ":")).encode("utf-8")
        self.send_frame(msg_id, body)

    def recv_frame(self) -> Tuple[int, Dict[str, Any]]:
        if self.sock is None:
            raise RuntimeError("socket is not connected")
        header = recv_exact(self.sock, 4)
        recv_msg_id, body_len = struct.unpack("!HH", header)
        body = recv_exact(self.sock, body_len)
        decoded = json.loads(body.decode("utf-8", errors="replace")) if body else {}
        if not isinstance(decoded, dict):
            raise ValueError(f"{self.label} received invalid JSON object")
        return recv_msg_id, decoded

    def recv_until(
        self,
        expected_ids: Sequence[int],
        overall_timeout: float,
        ignored_messages: Optional[List[Tuple[int, Dict[str, Any]]]] = None,
    ) -> Tuple[int, Dict[str, Any]]:
        deadline = time.time() + overall_timeout
        expected = set(int(one) for one in expected_ids)
        while time.time() < deadline:
            if self.sock is None:
                raise RuntimeError("socket is not connected")
            remaining = max(0.05, deadline - time.time())
            self.sock.settimeout(remaining)
            msg_id, body = self.recv_frame()
            if msg_id in expected:
                return msg_id, body
            if ignored_messages is not None:
                ignored_messages.append((msg_id, body))
        raise TimeoutError(f"{self.label} timed out waiting for {sorted(expected)}")


def chat_protocol_version(cfg: Dict[str, Any]) -> int:
    tcp_cfg = cfg.get("tcp_login", {})
    try:
        return max(2, int(tcp_cfg.get("protocol_version", 3)))
    except Exception:
        return 3


def relation_bootstrap_mode(cfg: Dict[str, Any]) -> str:
    if chat_protocol_version(cfg) >= 3:
        return "explicit_pull"
    return "inline"


def message_status_notifications_enabled(cfg: Dict[str, Any], scope: str = "") -> bool:
    feature_cfg = cfg.get("message_status_notifications", {})
    if isinstance(feature_cfg, dict):
        scoped = feature_cfg.get(str(scope).strip().lower(), None)
        if scoped is not None:
            return bool(scoped)
        default_value = feature_cfg.get("enabled", None)
        if default_value is not None:
            return bool(default_value)
    elif feature_cfg not in ({}, None):
        return bool(feature_cfg)
    return False


def chat_login_payload(cfg: Dict[str, Any], login_rsp: Dict[str, Any], trace_id: str) -> Dict[str, Any]:
    protocol_version = chat_protocol_version(cfg)
    payload: Dict[str, Any] = {
        "protocol_version": protocol_version,
        "trace_id": trace_id,
    }
    if protocol_version >= 3:
        payload["login_ticket"] = str(login_rsp.get("login_ticket", ""))
    else:
        payload["uid"] = int(login_rsp.get("uid", 0))
        payload["token"] = str(login_rsp.get("token", ""))
    return payload


def connect_chat_client(
    cfg: Dict[str, Any],
    account: Dict[str, Any],
    http_timeout: float,
    tcp_timeout: float,
    trace_id: str,
    label: str,
) -> Tuple[TcpChatClient, Dict[str, Any]]:
    status, login_rsp = gate_login(cfg, account["email"], account["password"], http_timeout, trace_id)
    if status != 200 or int(login_rsp.get("error", -1)) != 0:
        raise RuntimeError(f"gate login failed for {account['email']}: status={status}, body={login_rsp}")
    client = TcpChatClient(label, str(login_rsp.get("host", "127.0.0.1")), int(login_rsp.get("port", 0)), tcp_timeout)
    client.connect()
    payload = chat_login_payload(cfg, login_rsp, trace_id)
    if chat_protocol_version(cfg) >= 3 and not str(payload.get("login_ticket", "")).strip():
        client.close()
        raise RuntimeError(f"gate login missing login_ticket for {account['email']}: {login_rsp}")
    client.send_json(ID_CHAT_LOGIN, payload)
    recv_msg_id, body = client.recv_until([ID_CHAT_LOGIN_RSP], tcp_timeout)
    if recv_msg_id != ID_CHAT_LOGIN_RSP or int(body.get("error", -1)) != 0:
        client.close()
        raise RuntimeError(f"tcp chat login failed for {account['email']}: {body}")
    enriched = dict(account)
    enriched["uid"] = str(login_rsp.get("uid", ""))
    enriched["user_id"] = str(login_rsp.get("user_id", account.get("user_id", "")))
    enriched["token"] = str(login_rsp.get("token", ""))
    enriched["login_ticket"] = str(login_rsp.get("login_ticket", ""))
    enriched["chat_host"] = str(login_rsp.get("host", ""))
    enriched["chat_port"] = str(login_rsp.get("port", ""))
    enriched["chat_protocol_version"] = str(chat_protocol_version(cfg))
    return client, enriched


def build_private_text_payload(from_uid: int, to_uid: int, content: str, msg_id: Optional[str] = None) -> Dict[str, Any]:
    return {
        "fromuid": int(from_uid),
        "touid": int(to_uid),
        "text_array": [
            {
                "content": content,
                "msgid": msg_id or f"load-{uuid.uuid4().hex}",
                "created_at": now_ms(),
            }
        ],
    }


def build_group_message_payload(
    from_uid: int,
    group_id: int,
    content: str,
    msg_type: str = "text",
    file_name: str = "",
    mime: str = "",
    size: int = 0,
) -> Dict[str, Any]:
    msg_obj: Dict[str, Any] = {
        "msgid": f"load-{uuid.uuid4().hex}",
        "content": content,
        "msgtype": msg_type,
    }
    if file_name:
        msg_obj["file_name"] = file_name
    if mime:
        msg_obj["mime"] = mime
    if size > 0:
        msg_obj["size"] = int(size)
    return {
        "fromuid": int(from_uid),
        "groupid": int(group_id),
        "msg": msg_obj,
    }


def expect_response_status(
    rsp: Dict[str, Any],
    allowed_statuses: Sequence[str] = ("accepted", "persisted"),
) -> str:
    status = str(rsp.get("status", "")).strip().lower()
    if status not in {one.strip().lower() for one in allowed_statuses}:
        raise RuntimeError(f"unexpected response status: {rsp}")
    return status


def wait_for_message_status(
    client: "TcpChatClient",
    client_msg_id: str,
    scope: str,
    overall_timeout: float,
    expected_status: str = "persisted",
) -> Dict[str, Any]:
    deadline = time.time() + overall_timeout
    expected_status = str(expected_status).strip().lower()
    expected_scope = str(scope).strip().lower()
    while time.time() < deadline:
        remaining = max(0.05, deadline - time.time())
        msg_id, body = client.recv_until([ID_NOTIFY_MSG_STATUS_REQ], remaining)
        if msg_id != ID_NOTIFY_MSG_STATUS_REQ:
            continue
        if str(body.get("client_msg_id", "")).strip() != client_msg_id:
            continue
        if str(body.get("scope", "")).strip().lower() != expected_scope:
            continue
        if str(body.get("status", "")).strip().lower() != expected_status:
            raise RuntimeError(f"message status mismatch: {body}")
        if int(body.get("error", -1)) != 0:
            raise RuntimeError(f"message status error: {body}")
        return body
    raise TimeoutError(
        f"{client.label} timed out waiting for message status client_msg_id={client_msg_id} scope={scope} status={expected_status}"
    )


def maybe_wait_for_message_status(
    cfg: Dict[str, Any],
    client: "TcpChatClient",
    client_msg_id: str,
    scope: str,
    overall_timeout: float,
    expected_status: str = "persisted",
) -> Optional[Dict[str, Any]]:
    if not message_status_notifications_enabled(cfg, scope):
        return None
    return wait_for_message_status(client, client_msg_id, scope, overall_timeout, expected_status)


def encode_call_invite(call_type: str, join_url: str) -> str:
    payload = json.dumps({"type": call_type, "url": join_url}, separators=(",", ":")).encode("utf-8")
    return "__memochat_call__:" + base64.b64encode(payload).decode("ascii")


def default_join_url(call_type: str, uid_a: int, uid_b: int) -> str:
    first_uid = min(int(uid_a), int(uid_b))
    second_uid = max(int(uid_a), int(uid_b))
    room_suffix = uuid.uuid4().hex[:12]
    room_name = f"memochat-{call_type}-{first_uid}-{second_uid}-{room_suffix}"
    return f"https://meet.jit.si/{room_name}"


def report_path(report_prefix: str, explicit_path: str = "", cfg: Optional[Dict[str, Any]] = None) -> str:
    if explicit_path:
        return explicit_path
    return os.path.join(get_report_dir(cfg or {}), f"{report_prefix}_{utc_now_str()}.json")


def finalize_report(
    report_prefix: str,
    report: Dict[str, Any],
    explicit_path: str = "",
    cfg: Optional[Dict[str, Any]] = None,
) -> str:
    path = report_path(report_prefix, explicit_path, cfg)
    save_json(path, report)
    return path


def top_errors(error_counter: Counter, limit: int = 8) -> Dict[str, int]:
    return dict(error_counter.most_common(limit))


def run_parallel(total: int, concurrency: int, worker: Callable[[int], Dict[str, Any]]) -> Dict[str, Any]:
    latencies: List[float] = []
    success = 0
    failed = 0
    error_counter: Counter = Counter()
    phase_latencies: Dict[str, List[float]] = {}
    mutation_counter: Counter = Counter()
    samples: List[Dict[str, Any]] = []
    started = time.perf_counter()

    def consume(result: Dict[str, Any]) -> None:
        nonlocal success, failed
        latencies.append(float(result.get("elapsed_ms", 0.0)))
        if result.get("ok"):
            success += 1
        else:
            failed += 1
            error_counter[str(result.get("stage", "unknown"))] += 1
        for phase_name, elapsed_ms in result.get("phase_ms", {}).items():
            phase_latencies.setdefault(phase_name, []).append(float(elapsed_ms))
        for key, value in result.get("mutation", {}).items():
            mutation_counter[key] += int(value)
        sample = result.get("sample")
        if isinstance(sample, dict) and len(samples) < 12:
            samples.append(sample)

    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        for result in executor.map(worker, range(total)):
            consume(result)

    elapsed_sec = time.perf_counter() - started
    phase_breakdown: Dict[str, Dict[str, Any]] = {}
    for phase_name, phase_values in phase_latencies.items():
        phase_breakdown[phase_name] = build_summary(phase_values, len(phase_values), 0, elapsed_sec if elapsed_sec > 0 else 1.0)["latency_ms"]

    return {
        "summary": build_summary(latencies, success, failed, elapsed_sec),
        "error_counter": dict(error_counter),
        "top_errors": top_errors(error_counter),
        "phase_breakdown": phase_breakdown,
        "data_mutation_summary": dict(mutation_counter),
        "samples": samples,
    }


def query_one(conn, sql_text: str, params: Sequence[Any]) -> Optional[Dict[str, Any]]:
    with conn.cursor() as cursor:
        cursor.execute(sql_text, params)
        row = cursor.fetchone()
    return row


def query_all(conn, sql_text: str, params: Sequence[Any]) -> List[Dict[str, Any]]:
    with conn.cursor() as cursor:
        cursor.execute(sql_text, params)
        rows = cursor.fetchall()
    return list(rows or [])


def quoted_table_name(conn, table_name: str) -> str:
    module_name = str(type(conn).__module__).lower()
    if "psycopg" in module_name:
        return f'"{table_name}"'
    return f"`{table_name}`"


def get_user_by_email(conn, email: str) -> Optional[Dict[str, Any]]:
    return query_one(
        conn,
        f"SELECT uid, name, email, user_id, nick, icon FROM {quoted_table_name(conn, 'user')} WHERE email = %s LIMIT 1",
        [email],
    )


def get_user_by_uid(conn, uid: int) -> Optional[Dict[str, Any]]:
    return query_one(
        conn,
        f"SELECT uid, name, email, user_id, nick, icon FROM {quoted_table_name(conn, 'user')} WHERE uid = %s LIMIT 1",
        [uid],
    )


def is_friend(conn, uid_a: int, uid_b: int) -> bool:
    row = query_one(
        conn,
        "SELECT 1 AS ok FROM friend WHERE self_id = %s AND friend_id = %s LIMIT 1",
        [int(uid_a), int(uid_b)],
    )
    return bool(row)


def select_non_friend_pairs(conn, accounts: List[Dict[str, str]], pair_count: int) -> List[Tuple[Dict[str, str], Dict[str, str]]]:
    pairs: List[Tuple[Dict[str, str], Dict[str, str]]] = []
    for left_idx in range(len(accounts)):
        if len(pairs) >= pair_count:
            break
        left = accounts[left_idx]
        left_uid = int(left.get("uid", "0") or "0")
        if left_uid <= 0:
            continue
        for right_idx in range(left_idx + 1, len(accounts)):
            right = accounts[right_idx]
            right_uid = int(right.get("uid", "0") or "0")
            if right_uid <= 0:
                continue
            if is_friend(conn, left_uid, right_uid) or is_friend(conn, right_uid, left_uid):
                continue
            pairs.append((left, right))
            if len(pairs) >= pair_count:
                break
    return pairs


def send_search_user(client: TcpChatClient, target_user_id: str, timeout: float) -> Dict[str, Any]:
    client.send_json(ID_SEARCH_USER_REQ, {"user_id": target_user_id})
    _, body = client.recv_until([ID_SEARCH_USER_RSP], timeout)
    return body


def establish_friendship(
    requester_client: TcpChatClient,
    approver_client: TcpChatClient,
    requester: Dict[str, Any],
    approver: Dict[str, Any],
    timeout: float,
) -> Dict[str, Any]:
    phase_ms: Dict[str, float] = {}

    t0 = time.perf_counter()
    send_search_user(requester_client, str(approver["user_id"]), timeout)
    phase_ms["search_user"] = (time.perf_counter() - t0) * 1000.0

    t1 = time.perf_counter()
    requester_client.send_json(
        ID_ADD_FRIEND_REQ,
        {
            "uid": int(requester["uid"]),
            "applyname": requester.get("user", requester["email"].split("@", 1)[0]),
            "bakname": requester.get("user", requester["email"].split("@", 1)[0]),
            "touid": int(approver["uid"]),
            "labels": ["loadtest"],
        },
    )
    _, add_rsp = requester_client.recv_until([ID_ADD_FRIEND_RSP], timeout)
    phase_ms["friend_apply_rsp"] = (time.perf_counter() - t1) * 1000.0
    if int(add_rsp.get("error", -1)) != 0:
        raise RuntimeError(f"add friend failed: {add_rsp}")

    t2 = time.perf_counter()
    _, apply_notify = approver_client.recv_until([ID_NOTIFY_ADD_FRIEND_REQ], timeout)
    phase_ms["friend_apply_notify"] = (time.perf_counter() - t2) * 1000.0
    if int(apply_notify.get("error", -1)) != 0:
        raise RuntimeError(f"add friend notify failed: {apply_notify}")

    t3 = time.perf_counter()
    approver_client.send_json(
        ID_AUTH_FRIEND_REQ,
        {
            "fromuid": int(approver["uid"]),
            "touid": int(requester["uid"]),
            "back": requester.get("user", requester["email"].split("@", 1)[0]),
            "labels": ["loadtest"],
        },
    )
    _, auth_rsp = approver_client.recv_until([ID_AUTH_FRIEND_RSP], timeout)
    phase_ms["friend_auth_rsp"] = (time.perf_counter() - t3) * 1000.0
    if int(auth_rsp.get("error", -1)) != 0:
        raise RuntimeError(f"auth friend failed: {auth_rsp}")

    t4 = time.perf_counter()
    _, auth_notify = requester_client.recv_until([ID_NOTIFY_AUTH_FRIEND_REQ], timeout)
    phase_ms["friend_auth_notify"] = (time.perf_counter() - t4) * 1000.0
    if int(auth_notify.get("error", -1)) != 0:
        raise RuntimeError(f"auth friend notify failed: {auth_notify}")

    return phase_ms


def build_group_create_payload(owner_uid: int, group_name: str, member_user_ids: Optional[List[str]] = None) -> Dict[str, Any]:
    return {
        "fromuid": int(owner_uid),
        "name": group_name,
        "member_limit": 200,
        "member_user_ids": list(member_user_ids or []),
    }


def db_verify_friendship(conn, uid_a: int, uid_b: int) -> bool:
    return is_friend(conn, uid_a, uid_b) and is_friend(conn, uid_b, uid_a)


def refresh_account_profiles(cfg: Dict[str, Any], accounts: List[Dict[str, str]]) -> List[Dict[str, str]]:
    conn = open_mysql(cfg)
    try:
        refreshed: List[Dict[str, str]] = []
        for account in accounts:
            enriched = dict(account)
            row = get_user_by_email(conn, account["email"])
            if row:
                enriched["uid"] = str(row.get("uid", ""))
                enriched["user"] = str(row.get("name", account.get("user", "")))
                enriched["user_id"] = str(row.get("user_id", account.get("user_id", "")))
            refreshed.append(enriched)
        return refreshed
    finally:
        conn.close()


def ensure_accounts(
    cfg: Dict[str, Any],
    required_count: int,
    csv_path: str,
    logger: logging.Logger,
    seed_prefix: str,
) -> List[Dict[str, str]]:
    timeout = float(cfg.get("default_http_timeout_sec", 8))
    accounts = load_accounts(cfg, csv_path)
    accounts = refresh_account_profiles(cfg, accounts) if accounts else []
    accounts = filter_login_ready_accounts(cfg, accounts, timeout=timeout, logger=logger) if accounts else []
    domain = str(cfg.get("account_domain", "loadtest.local"))
    generated: List[Dict[str, str]] = []
    while len(accounts) + len(generated) < required_count:
        account = generate_account(seed_prefix, domain)
        verify_trace = new_trace_id()
        _, verify_rsp = request_verify_code(cfg, account["email"], timeout, verify_trace)
        if int(verify_rsp.get("error", -1)) != 0:
            logger.warning(
                "verify code request failed while ensuring accounts",
                extra={
                    "event": "loadtest.ensure_accounts.verify_failed",
                    "trace_id": verify_trace,
                    "account_email": account["email"],
                    "payload": {"response": verify_rsp},
                },
            )
            continue
        code = fetch_verify_code_from_redis(cfg, account["email"])
        register_trace = new_trace_id()
        _, register_rsp = register_via_gate(
            cfg,
            account["user"],
            account["email"],
            account["password"],
            code,
            timeout,
            register_trace,
        )
        if int(register_rsp.get("error", -1)) != 0:
            logger.warning(
                "register failed while ensuring accounts",
                extra={
                    "event": "loadtest.ensure_accounts.register_failed",
                    "trace_id": register_trace,
                    "account_email": account["email"],
                    "payload": {"response": register_rsp},
                },
            )
            continue
        account["uid"] = str(register_rsp.get("uid", ""))
        account["user_id"] = str(register_rsp.get("user_id", ""))
        generated.append(account)

    merged = upsert_accounts(accounts, generated)
    if csv_path:
        save_accounts_csv(csv_path, merged)
    return merged


def pair_accounts(accounts: List[Dict[str, str]], pair_count: int) -> List[Tuple[Dict[str, str], Dict[str, str]]]:
    pairs: List[Tuple[Dict[str, str], Dict[str, str]]] = []
    cursor = 0
    while len(pairs) < pair_count and cursor + 1 < len(accounts):
        pairs.append((accounts[cursor], accounts[cursor + 1]))
        cursor += 2
    return pairs


def chunked(seq: Sequence[Any], size: int) -> Iterable[Sequence[Any]]:
    for idx in range(0, len(seq), size):
        yield seq[idx: idx + size]
