from __future__ import annotations

import contextlib
import hashlib
import os
import random
import sys
from dataclasses import dataclass
from typing import Any, Iterator

import structlog

from config import LangSmithConfig, ObservabilityConfig, settings

logger = structlog.get_logger()

SENSITIVE_KEYS = {
    "api_key",
    "apikey",
    "authorization",
    "cookie",
    "content",
    "email",
    "password",
    "passwd",
    "query",
    "secret",
    "secret_key",
    "token",
}


@dataclass(frozen=True)
class LangSmithStatus:
    enabled: bool
    project: str = ""
    endpoint: str = ""
    reason: str = ""


def init_langsmith(config: ObservabilityConfig | None = None) -> LangSmithStatus:
    config = config or settings.observability
    langsmith_cfg = config.langsmith
    if not _langsmith_enabled(config):
        return LangSmithStatus(enabled=False, reason="disabled")

    api_key = os.getenv(langsmith_cfg.api_key_env, "")
    if not api_key:
        logger.warning("langsmith.disabled_missing_api_key", api_key_env=langsmith_cfg.api_key_env)
        return LangSmithStatus(enabled=False, project=langsmith_cfg.project, reason="missing_api_key")

    project = os.getenv("LANGSMITH_PROJECT", langsmith_cfg.project)
    endpoint = os.getenv("LANGSMITH_ENDPOINT", langsmith_cfg.endpoint)

    tracing = _effective_tracing(langsmith_cfg.tracing)
    os.environ["LANGSMITH_TRACING"] = "true" if tracing else "false"
    os.environ["LANGSMITH_API_KEY"] = api_key
    os.environ["LANGSMITH_PROJECT"] = project
    os.environ["LANGSMITH_ENDPOINT"] = endpoint
    logger.info(
        "langsmith.initialized",
        project=project,
        endpoint=endpoint,
        tracing=tracing,
    )
    return LangSmithStatus(enabled=True, project=project, endpoint=endpoint)


def _langsmith_enabled(config: ObservabilityConfig) -> bool:
    return (
        bool(config.enabled)
        and config.backend.lower() in {"langsmith", "both"}
        and _effective_tracing(config.langsmith.tracing)
        and _sampled(config.langsmith.sampling_rate)
    )


def _effective_tracing(default: bool) -> bool:
    raw = os.getenv("LANGSMITH_TRACING")
    if raw is None:
        return bool(default)
    normalized = raw.strip().lower()
    if normalized in {"1", "true", "yes", "on"}:
        return True
    if normalized in {"0", "false", "no", "off"}:
        return False
    return bool(default)


def _sampled(rate: float) -> bool:
    if rate >= 1.0:
        return True
    if rate <= 0.0:
        return False
    return random.random() < rate


def uid_hash(uid: Any) -> str:
    raw = str(uid).encode("utf-8", errors="ignore")
    return hashlib.sha256(raw).hexdigest()[:16]


def sanitize_payload(value: Any, config: LangSmithConfig | None = None) -> Any:
    config = config or settings.observability.langsmith
    if isinstance(value, dict):
        sanitized: dict[str, Any] = {}
        for key, item in value.items():
            lowered = str(key).lower()
            if lowered == "uid":
                sanitized["uid_hash"] = uid_hash(item)
                continue
            if _should_redact_key(lowered, config):
                sanitized[str(key)] = "[redacted]"
                continue
            sanitized[str(key)] = sanitize_payload(item, config)
        return sanitized
    if isinstance(value, list):
        return [sanitize_payload(item, config) for item in value]
    return value


def _should_redact_key(key: str, config: LangSmithConfig) -> bool:
    if not config.redact_inputs:
        return False
    if key in {"content", "query"} and config.upload_user_content:
        return False
    return key in SENSITIVE_KEYS or any(token in key for token in ("secret", "token", "password", "api_key"))


def trace_metadata(extra: dict[str, Any] | None = None) -> dict[str, Any]:
    metadata = {
        "service": "AIOrchestrator",
        "env": os.getenv("MEMOCHAT_ENV", "local"),
    }
    if extra:
        metadata.update(sanitize_payload(extra))
    return metadata


@contextlib.contextmanager
def trace_context(
    name: str,
    config: ObservabilityConfig | None = None,
    *,
    run_type: str = "chain",
    inputs: dict[str, Any] | None = None,
    metadata: dict[str, Any] | None = None,
    tags: list[str] | None = None,
) -> Iterator[Any]:
    config = config or settings.observability
    if not _langsmith_enabled(config) or not os.getenv(config.langsmith.api_key_env, ""):
        yield None
        return

    try:
        from langsmith.run_helpers import trace
    except Exception as exc:
        logger.warning("langsmith.trace_unavailable", error=str(exc))
        yield None
        return

    clean_inputs = sanitize_payload(inputs or {}, config.langsmith)
    clean_metadata = trace_metadata(metadata)
    run_tags = list(config.langsmith.tags) + list(tags or [])
    project = os.getenv("LANGSMITH_PROJECT", config.langsmith.project)
    try:
        trace_manager = trace(
            name=name,
            run_type=run_type,
            inputs=clean_inputs,
            project_name=project,
            metadata=clean_metadata,
            tags=run_tags,
        )
        run = trace_manager.__enter__()
    except Exception as exc:
        logger.warning("langsmith.trace_start_failed", error=str(exc))
        yield None
        return

    try:
        yield run
    except BaseException:
        exc_info = sys.exc_info()
        try:
            trace_manager.__exit__(*exc_info)
        except Exception as exc:
            logger.warning("langsmith.trace_finish_failed", error=str(exc))
        raise
    else:
        try:
            trace_manager.__exit__(None, None, None)
        except Exception as exc:
            logger.warning("langsmith.trace_finish_failed", error=str(exc))


def traceable(name: str, *, run_type: str = "chain", tags: list[str] | None = None):
    def decorator(fn):
        try:
            from langsmith import traceable as langsmith_traceable
        except Exception:
            return fn

        return langsmith_traceable(name=name, run_type=run_type, tags=tags)(fn)

    return decorator


def set_run_output(run: Any, output: Any, config: ObservabilityConfig | None = None) -> None:
    if run is None:
        return
    config = config or settings.observability
    if config.langsmith.redact_outputs:
        output = "[redacted]"
    try:
        run.end(outputs=sanitize_payload({"output": output}, config.langsmith))
    except Exception as exc:
        logger.warning("langsmith.set_output_failed", error=str(exc))


def set_run_error(run: Any, exc: Exception) -> None:
    if run is None:
        return
    try:
        run.end(error=f"{type(exc).__name__}: {str(exc)[:500]}")
    except Exception as end_exc:
        logger.warning("langsmith.set_error_failed", error=str(end_exc))
