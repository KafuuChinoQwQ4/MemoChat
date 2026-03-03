import csv
import datetime as _dt
import json
import logging
import math
import os
import threading
import uuid
from logging.handlers import TimedRotatingFileHandler
from typing import Any, Dict, List


def utc_now_str() -> str:
    return _dt.datetime.utcnow().strftime('%Y%m%d_%H%M%S')


def ensure_dir(path: str) -> None:
    if path:
        os.makedirs(path, exist_ok=True)


def load_json(path: str) -> Dict[str, Any]:
    with open(path, 'r', encoding='utf-8-sig') as f:
        return json.load(f)


def save_json(path: str, payload: Dict[str, Any]) -> None:
    ensure_dir(os.path.dirname(path))
    with open(path, 'w', encoding='utf-8-sig') as f:
        json.dump(payload, f, ensure_ascii=False, indent=2)


def xor_encode(raw: str) -> str:
    x = len(raw) % 255
    return ''.join(chr(ord(ch) ^ x) for ch in raw)


def load_accounts(config: Dict[str, Any], csv_path: str = '') -> List[Dict[str, str]]:
    accounts = []
    if csv_path:
        with open(csv_path, 'r', encoding='utf-8-sig', newline='') as f:
            reader = csv.DictReader(f)
            for row in reader:
                email = (row.get('email') or '').strip()
                password = (row.get('password') or '').strip()
                if email and password:
                    accounts.append({'email': email, 'password': password})
    else:
        for it in config.get('accounts', []):
            email = str(it.get('email', '')).strip()
            password = str(it.get('password', '')).strip()
            if email and password:
                accounts.append({'email': email, 'password': password})
    return accounts


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
        'total': total,
        'success': success,
        'failed': failed,
        'success_rate': round((success / total) if total else 0.0, 6),
        'elapsed_sec': round(elapsed_sec, 6),
        'throughput_rps': round((total / elapsed_sec) if elapsed_sec > 0 else 0.0, 3),
        'latency_ms': {
            'min': round(min(latencies_ms), 3) if latencies_ms else 0.0,
            'avg': round((sum(latencies_ms) / len(latencies_ms)), 3) if latencies_ms else 0.0,
            'p50': round(pct(latencies_ms, 0.50), 3) if latencies_ms else 0.0,
            'p90': round(pct(latencies_ms, 0.90), 3) if latencies_ms else 0.0,
            'p95': round(pct(latencies_ms, 0.95), 3) if latencies_ms else 0.0,
            'p99': round(pct(latencies_ms, 0.99), 3) if latencies_ms else 0.0,
            'max': round(max(latencies_ms), 3) if latencies_ms else 0.0,
        },
    }


class JsonFormatter(logging.Formatter):
    def format(self, record: logging.LogRecord) -> str:
        payload = {
            'ts': _dt.datetime.utcnow().isoformat(timespec='milliseconds') + 'Z',
            'level': record.levelname.lower(),
            'service': getattr(record, 'service', record.name),
            'event': getattr(record, 'event', record.name),
            'message': record.getMessage(),
            'thread': threading.current_thread().name,
        }
        trace_id = getattr(record, 'trace_id', '')
        if trace_id:
            payload['trace_id'] = trace_id
        extra_payload = getattr(record, 'payload', None)
        if isinstance(extra_payload, dict):
            payload.update(extra_payload)
        return json.dumps(payload, ensure_ascii=False)


def init_json_logger(name: str, log_dir: str = 'logs', to_console: bool = True,
                     level: str = 'INFO') -> logging.Logger:
    ensure_dir(log_dir)
    logger = logging.getLogger(name)
    logger.setLevel(getattr(logging, level.upper(), logging.INFO))
    logger.propagate = False
    if logger.handlers:
        return logger

    formatter = JsonFormatter()

    file_handler = TimedRotatingFileHandler(
        filename=os.path.join(log_dir, f'{name}.json'),
        when='midnight',
        backupCount=14,
        encoding='utf-8',
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
    return str(uuid.uuid4())
