from __future__ import annotations

import asyncio
import hashlib
import json
import math
import re
import struct
import time
import uuid
from dataclasses import dataclass, field
from typing import Any

import structlog
from config import settings
from observability.metrics import ai_metrics

try:
    from redis import asyncio as redis_async
    from redis.exceptions import RedisError, ResponseError
except Exception:  # pragma: no cover - runtime dependency is optional in CI/local tools
    redis_async = None

    class RedisError(Exception):
        pass

    class ResponseError(RedisError):
        pass


logger = structlog.get_logger()

_DISABLE_FLAGS = {
    "no_cache",
    "cache_skip",
    "disable_cache",
    "semantic_cache_skip",
    "semantic_cache_disable",
    "disable_semantic_cache",
}
_FORCE_FLAGS = {"semantic_cache_force", "force_semantic_cache"}
_VOLATILE_KEYWORDS = (
    "最新",
    "新闻",
    "实时",
    "今天",
    "现在",
    "联网",
    "搜索",
    "latest",
    "recent",
    "today",
    "news",
    "search",
    "current",
)
_CJK_TEXT_RE = re.compile(r"[\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff]")
_CJK_INNER_SPACE_RE = re.compile(
    r"(?<=[\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff])\s+(?=[\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff])"
)
_TERMINAL_PUNCTUATION = "?？!！。."


@dataclass
class SemanticCacheHit:
    key: str
    answer: str
    similarity: float
    distance: float
    model: str = ""
    tokens: int = 0
    trace_id: str = ""
    skill: str = ""
    question: str = ""
    cached_at: int = 0
    match_kind: str = "semantic"
    metadata: dict[str, Any] = field(default_factory=dict)


@dataclass
class SemanticCacheLookup:
    hit: SemanticCacheHit | None = None
    vector: list[float] | None = None
    status: str = "skip"
    reason: str = ""
    match_kind: str = ""


@dataclass
class _CacheDecision:
    cacheable: bool
    reason: str
    scope_hash: str = ""
    skill_hash: str = ""
    variant_hash: str = ""
    normalized_content: str = ""
    metadata: dict[str, Any] = field(default_factory=dict)


def _now_ms() -> int:
    return int(time.time() * 1000)


def _truthy(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.strip().lower() in {"1", "true", "yes", "on"}
    return bool(value)


def _request_metadata(request) -> dict[str, Any]:
    metadata = getattr(request, "metadata", {})
    return metadata if isinstance(metadata, dict) else {}


def _normalize_text(value: str) -> str:
    return re.sub(r"\s+", " ", str(value or "")).strip()


def _normalize_exact_text(value: str) -> str:
    text = _normalize_text(value).casefold()
    if _CJK_TEXT_RE.search(text):
        text = _CJK_INNER_SPACE_RE.sub("", text)
    return text.strip().rstrip(_TERMINAL_PUNCTUATION).strip()


def _hash_tag(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def _decode(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return str(value)


def _safe_int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def _safe_float(value: Any, default: float = 0.0) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


class SemanticCacheService:
    def __init__(self, embedder, config=None):
        self._embedder = embedder
        self._config = config or settings.semantic_cache
        self._redis = None
        self._ready = False
        self._disabled_reason = ""

    @property
    def ready(self) -> bool:
        return self._ready

    @property
    def persist_hits_to_memory(self) -> bool:
        return bool(self._config.persist_hits_to_memory)

    async def startup(self) -> None:
        if not self._config.enabled:
            self._disabled_reason = "disabled_by_config"
            return
        if redis_async is None:
            self._disabled_reason = "redis_client_missing"
            ai_metrics.semantic_cache.inc(status="error", reason=self._disabled_reason)
            logger.warning("semantic_cache.disabled", reason=self._disabled_reason)
            return

        redis_config = self._config.redis
        try:
            self._redis = redis_async.Redis(
                host=redis_config.host,
                port=redis_config.port,
                username=redis_config.username or None,
                password=redis_config.password or None,
                db=redis_config.db,
                ssl=redis_config.ssl,
                decode_responses=False,
                socket_timeout=redis_config.socket_timeout_sec,
                socket_connect_timeout=redis_config.connect_timeout_sec,
            )
            await self._redis.ping()
            await self._ensure_index()
            self._ready = True
            self._disabled_reason = ""
            logger.info(
                "semantic_cache.ready",
                index=self._config.index_name,
                redis=f"{redis_config.host}:{redis_config.port}",
                threshold=self._config.similarity_threshold,
            )
        except ResponseError as exc:
            self._disabled_reason = self._classify_response_error(exc)
            ai_metrics.semantic_cache.inc(status="error", reason=self._disabled_reason)
            logger.warning("semantic_cache.disabled", reason=self._disabled_reason, error=str(exc))
            await self.shutdown()
        except Exception as exc:
            self._disabled_reason = type(exc).__name__
            ai_metrics.semantic_cache.inc(status="error", reason=self._disabled_reason)
            logger.warning("semantic_cache.disabled", reason=self._disabled_reason, error=str(exc))
            await self.shutdown()

    async def shutdown(self) -> None:
        self._ready = False
        client = self._redis
        self._redis = None
        if client is None:
            return
        close = getattr(client, "aclose", None) or getattr(client, "close", None)
        if close is not None:
            result = close()
            if hasattr(result, "__await__"):
                await result

    async def lookup(self, request, skill, plan_steps) -> SemanticCacheLookup:
        decision = self._decision(request, skill, plan_steps)
        if not decision.cacheable:
            ai_metrics.semantic_cache.inc(status="skip", reason=decision.reason)
            return SemanticCacheLookup(status="skip", reason=decision.reason)
        if not self._ready or self._redis is None:
            reason = self._disabled_reason or "not_ready"
            ai_metrics.semantic_cache.inc(status="skip", reason=reason)
            return SemanticCacheLookup(status="skip", reason=reason)

        exact_hit = await self._lookup_exact(decision)
        if exact_hit is not None:
            ai_metrics.semantic_cache.inc(status="hit", reason="exact")
            if exact_hit.tokens > 0:
                ai_metrics.semantic_cache_saved_tokens.inc(exact_hit.tokens)
            asyncio.create_task(self._record_hit(exact_hit))
            return SemanticCacheLookup(hit=exact_hit, status="hit", reason="ok", match_kind="exact")

        try:
            vector = await self._embed(decision.normalized_content)
            response = await self._redis.execute_command(
                "FT.SEARCH",
                self._config.index_name,
                (
                    f"(@scope_hash:{{{decision.scope_hash}}} "
                    f"@skill_hash:{{{decision.skill_hash}}} "
                    f"@variant_hash:{{{decision.variant_hash}}})"
                    f"=>[KNN {max(int(self._config.top_k), 1)} @embedding $vector AS vector_distance]"
                ),
                "PARAMS",
                "2",
                "vector",
                self._vector_bytes(vector),
                "SORTBY",
                "vector_distance",
                "ASC",
                "RETURN",
                "12",
                "answer",
                "question",
                "model",
                "tokens",
                "trace_id",
                "skill",
                "created_at",
                "metadata_json",
                "hit_count",
                "vector_distance",
                "match_kind",
                "question_hash",
                "DIALECT",
                "2",
            )
            hit = self._parse_hit(response)
            if hit is None:
                ai_metrics.semantic_cache.inc(status="miss", reason="not_found")
                return SemanticCacheLookup(vector=vector, status="miss", reason="not_found")

            if hit.similarity < float(self._config.similarity_threshold):
                ai_metrics.semantic_cache.inc(status="miss", reason="below_threshold")
                return SemanticCacheLookup(vector=vector, status="miss", reason="below_threshold")

            ai_metrics.semantic_cache.inc(status="hit", reason=hit.match_kind or "semantic")
            if hit.tokens > 0:
                ai_metrics.semantic_cache_saved_tokens.inc(hit.tokens)
            asyncio.create_task(self._record_hit(hit))
            return SemanticCacheLookup(hit=hit, vector=vector, status="hit", reason="ok", match_kind=hit.match_kind)
        except Exception as exc:
            ai_metrics.semantic_cache.inc(status="error", reason=type(exc).__name__)
            logger.warning("semantic_cache.lookup_failed", error=str(exc))
            return SemanticCacheLookup(status="error", reason=type(exc).__name__)

    async def store(
        self,
        request,
        skill,
        plan_steps,
        *,
        answer: str,
        model: str,
        tokens: int,
        trace_id: str,
        feedback_summary: str = "",
        vector: list[float] | None = None,
    ) -> None:
        decision = self._decision(request, skill, plan_steps)
        if not decision.cacheable:
            ai_metrics.semantic_cache.inc(status="skip", reason=f"store_{decision.reason}")
            return
        if not self._ready or self._redis is None:
            return

        answer_text = str(answer or "").strip()
        if not answer_text:
            ai_metrics.semantic_cache.inc(status="skip", reason="empty_answer")
            return

        key = f"{self._config.key_prefix}item:{uuid.uuid4().hex}"
        stored_at = _now_ms()
        if self._exact_match_enabled():
            try:
                await self._store_exact(
                    request,
                    decision,
                    skill,
                    answer_text=answer_text,
                    model=model,
                    tokens=tokens,
                    trace_id=trace_id,
                    feedback_summary=feedback_summary,
                    item_key=key,
                    stored_at=stored_at,
                )
            except Exception as exc:
                ai_metrics.semantic_cache.inc(status="error", reason=f"exact_{type(exc).__name__}")
                logger.warning("semantic_cache.exact_store_failed", error=str(exc))

        try:
            if vector is None:
                vector = await self._embed(decision.normalized_content)
            await self._redis.hset(
                key,
                mapping=self._cache_mapping(
                    request,
                    decision,
                    skill,
                    answer_text=answer_text,
                    model=model,
                    tokens=tokens,
                    trace_id=trace_id,
                    feedback_summary=feedback_summary,
                    item_key=key,
                    stored_at=stored_at,
                    match_kind="semantic",
                    vector=vector,
                ),
            )
            if self._config.ttl_sec > 0:
                await self._redis.expire(key, int(self._config.ttl_sec))
            ai_metrics.semantic_cache.inc(status="store", reason="ok")
        except Exception as exc:
            ai_metrics.semantic_cache.inc(status="error", reason=type(exc).__name__)
            logger.warning("semantic_cache.store_failed", error=str(exc))

    async def _ensure_index(self) -> None:
        assert self._redis is not None
        try:
            await self._redis.execute_command("FT.INFO", self._config.index_name)
            return
        except ResponseError as exc:
            reason = self._classify_response_error(exc)
            if reason not in {"unknown_index", "index_missing"}:
                raise

        await self._redis.execute_command(
            "FT.CREATE",
            self._config.index_name,
            "ON",
            "HASH",
            "PREFIX",
            "1",
            f"{self._config.key_prefix}item:",
            "SCHEMA",
            "scope_hash",
            "TAG",
            "skill_hash",
            "TAG",
            "variant_hash",
            "TAG",
            "question",
            "TEXT",
            "embedding",
            "VECTOR",
            "HNSW",
            "6",
            "TYPE",
            "FLOAT32",
            "DIM",
            str(int(self._config.dimension)),
            "DISTANCE_METRIC",
            "COSINE",
        )

    async def _embed(self, content: str) -> list[float]:
        vector = await self._embedder.embed_query(content)
        if len(vector) != int(self._config.dimension):
            raise ValueError(f"embedding dimension mismatch: {len(vector)} != {self._config.dimension}")
        cleaned: list[float] = []
        for value in vector:
            number = float(value)
            cleaned.append(number if math.isfinite(number) else 0.0)
        return cleaned

    def _decision(self, request, skill, plan_steps) -> _CacheDecision:
        metadata = _request_metadata(request)
        force = any(_truthy(metadata.get(flag)) for flag in _FORCE_FLAGS)
        if any(_truthy(metadata.get(flag)) for flag in _DISABLE_FLAGS):
            return _CacheDecision(False, "metadata_disabled")

        content = _normalize_text(getattr(request, "content", ""))
        if len(content) < self._config.min_question_chars:
            return _CacheDecision(False, "question_too_short")
        if len(content) > self._config.max_question_chars:
            return _CacheDecision(False, "question_too_long")

        actions = {str(getattr(step, "action", "") or "") for step in (plan_steps or [])}
        requested_tools = [
            str(tool).strip() for tool in (getattr(request, "requested_tools", []) or []) if str(tool).strip()
        ]
        if not force and requested_tools and not self._config.cache_tool_results:
            return _CacheDecision(False, "requested_tools")
        if not force and actions and not self._config.cache_tool_results:
            return _CacheDecision(False, "plan_uses_tools")

        blocked_actions = {str(action) for action in self._config.non_cacheable_actions}
        if not force and actions.intersection(blocked_actions):
            return _CacheDecision(False, "non_cacheable_action")
        lowered = content.lower()
        if (
            not force
            and self._config.skip_volatile_inputs
            and any(keyword in content or keyword in lowered for keyword in _VOLATILE_KEYWORDS)
        ):
            return _CacheDecision(False, "volatile_input")

        scope_key = self._scope_key(request, metadata)
        skill_name = getattr(skill, "name", "") or "default"
        variant_key = self._variant_key(request, metadata, actions, requested_tools)
        return _CacheDecision(
            True,
            "ok",
            scope_hash=_hash_tag(scope_key),
            skill_hash=_hash_tag(skill_name),
            variant_hash=_hash_tag(variant_key),
            normalized_content=content,
            metadata=metadata,
        )

    def _scope_key(self, request, metadata: dict[str, Any]) -> str:
        scope = str(self._config.scope or "user").strip().lower()
        uid = getattr(request, "uid", 0)
        if scope == "global":
            return "global"
        if scope == "tenant":
            tenant = str(metadata.get("tenant_id") or metadata.get("org_id") or uid)
            return f"tenant:{tenant}"
        if scope == "session":
            return f"user:{uid}:session:{getattr(request, 'session_id', '')}"
        return f"user:{uid}"

    def _variant_key(self, request, metadata: dict[str, Any], actions: set[str], requested_tools: list[str]) -> str:
        parts: dict[str, Any] = {
            "target_lang": getattr(request, "target_lang", ""),
            "namespace": metadata.get("semantic_cache_namespace", ""),
            "prompt_version": metadata.get("semantic_cache_prompt_version", ""),
        }
        if self._config.include_model_in_cache_key:
            parts.update(
                {
                    "model_type": getattr(request, "model_type", ""),
                    "model_name": getattr(request, "model_name", ""),
                    "deployment_preference": getattr(request, "deployment_preference", ""),
                }
            )
        if actions or requested_tools:
            parts.update(
                {
                    "actions": sorted(actions),
                    "requested_tools": sorted(requested_tools),
                    "tool_arguments": getattr(request, "tool_arguments", {}) or {},
                }
            )
        return json.dumps(parts, ensure_ascii=False, sort_keys=True, default=str)

    def _vector_bytes(self, vector: list[float]) -> bytes:
        return struct.pack(f"<{len(vector)}f", *[float(value) for value in vector])

    def _exact_match_enabled(self) -> bool:
        return bool(getattr(self._config, "exact_match_enabled", True))

    def _exact_cache_key(self, decision: _CacheDecision) -> str:
        question_hash = self._question_hash(decision)
        return (
            f"{self._config.key_prefix}exact:"
            f"{decision.scope_hash}:{decision.skill_hash}:{decision.variant_hash}:{question_hash}"
        )

    def _question_hash(self, decision: _CacheDecision) -> str:
        exact_text = _normalize_exact_text(decision.normalized_content)
        if not exact_text:
            exact_text = _normalize_text(decision.normalized_content).casefold()
        return _hash_tag(exact_text)

    def _cache_mapping(
        self,
        request,
        decision: _CacheDecision,
        skill,
        *,
        answer_text: str,
        model: str,
        tokens: int,
        trace_id: str,
        feedback_summary: str,
        item_key: str,
        stored_at: int,
        match_kind: str,
        vector: list[float] | None = None,
    ) -> dict[str, Any]:
        question_hash = self._question_hash(decision)
        metadata_json = json.dumps(
            {
                "uid": getattr(request, "uid", 0),
                "session_id": getattr(request, "session_id", ""),
                "target_lang": getattr(request, "target_lang", ""),
                "model_type": getattr(request, "model_type", ""),
                "model_name": getattr(request, "model_name", ""),
                "deployment_preference": getattr(request, "deployment_preference", ""),
                "namespace": decision.metadata.get("semantic_cache_namespace", ""),
                "feedback_summary": feedback_summary,
                "match_kind": match_kind,
                "item_key": item_key,
                "question_hash": question_hash,
            },
            ensure_ascii=False,
        )
        mapping: dict[str, Any] = {
            "scope_hash": decision.scope_hash,
            "skill_hash": decision.skill_hash,
            "variant_hash": decision.variant_hash,
            "question_hash": question_hash,
            "question": decision.normalized_content[: self._config.max_question_chars],
            "answer": answer_text[: self._config.max_answer_chars],
            "model": model or "",
            "tokens": str(max(_safe_int(tokens, 0), 0)),
            "trace_id": trace_id or "",
            "skill": getattr(skill, "name", ""),
            "created_at": str(stored_at),
            "hit_count": "0",
            "metadata_json": metadata_json,
            "match_kind": match_kind,
            "item_key": item_key,
        }
        if match_kind == "exact":
            mapping["similarity"] = "1.0"
            mapping["vector_distance"] = "0.0"
        if vector is not None:
            mapping["embedding"] = self._vector_bytes(vector)
        return mapping

    async def _lookup_exact(self, decision: _CacheDecision) -> SemanticCacheHit | None:
        if not self._exact_match_enabled() or self._redis is None:
            return None
        key = self._exact_cache_key(decision)
        try:
            raw = await self._redis.hgetall(key)
        except Exception as exc:
            ai_metrics.semantic_cache.inc(status="error", reason=f"exact_{type(exc).__name__}")
            logger.warning("semantic_cache.exact_lookup_failed", error=str(exc))
            return None
        if not raw:
            return None
        if not isinstance(raw, dict):
            return None
        data = {_decode(name): value for name, value in raw.items()}
        return self._hit_from_data(
            key,
            data,
            similarity=1.0,
            distance=0.0,
            match_kind="exact",
        )

    async def _store_exact(
        self,
        request,
        decision: _CacheDecision,
        skill,
        *,
        answer_text: str,
        model: str,
        tokens: int,
        trace_id: str,
        feedback_summary: str,
        item_key: str,
        stored_at: int,
    ) -> None:
        assert self._redis is not None
        key = self._exact_cache_key(decision)
        await self._redis.hset(
            key,
            mapping=self._cache_mapping(
                request,
                decision,
                skill,
                answer_text=answer_text,
                model=model,
                tokens=tokens,
                trace_id=trace_id,
                feedback_summary=feedback_summary,
                item_key=item_key,
                stored_at=stored_at,
                match_kind="exact",
            ),
        )
        if self._config.ttl_sec > 0:
            await self._redis.expire(key, int(self._config.ttl_sec))

    def _parse_hit(self, response: Any) -> SemanticCacheHit | None:
        if not isinstance(response, (list, tuple)) or len(response) < 3:
            return None
        total = _safe_int(response[0], 0)
        if total <= 0:
            return None

        key = _decode(response[1])
        fields = response[2]
        if not isinstance(fields, (list, tuple)):
            return None
        data: dict[str, Any] = {}
        for index in range(0, len(fields) - 1, 2):
            data[_decode(fields[index])] = fields[index + 1]

        distance = _safe_float(_decode(data.get("vector_distance")), 1.0)
        similarity = max(0.0, min(1.0, 1.0 - distance))
        return self._hit_from_data(
            key,
            data,
            similarity=similarity,
            distance=distance,
            match_kind="semantic",
        )

    def _hit_from_data(
        self,
        key: str,
        data: dict[str, Any],
        *,
        similarity: float,
        distance: float,
        match_kind: str,
    ) -> SemanticCacheHit | None:
        answer = _decode(data.get("answer")).strip()
        if not answer:
            return None
        metadata_json = _decode(data.get("metadata_json"))
        try:
            metadata = json.loads(metadata_json) if metadata_json else {}
        except json.JSONDecodeError:
            metadata = {}
        if not isinstance(metadata, dict):
            metadata = {}
        hit_match_kind = _decode(data.get("match_kind")) or match_kind
        metadata["hit_count"] = _safe_int(_decode(data.get("hit_count")), 0)
        metadata["match_kind"] = hit_match_kind
        item_key = _decode(data.get("item_key"))
        question_hash = _decode(data.get("question_hash"))
        if item_key:
            metadata["item_key"] = item_key
        if question_hash:
            metadata["question_hash"] = question_hash
        return SemanticCacheHit(
            key=key,
            answer=answer,
            similarity=similarity,
            distance=distance,
            model=_decode(data.get("model")),
            tokens=_safe_int(_decode(data.get("tokens")), 0),
            trace_id=_decode(data.get("trace_id")),
            skill=_decode(data.get("skill")),
            question=_decode(data.get("question")),
            cached_at=_safe_int(_decode(data.get("created_at")), 0),
            match_kind=hit_match_kind,
            metadata=metadata,
        )

    async def _record_hit(self, hit: SemanticCacheHit | str) -> None:
        key = hit.key if isinstance(hit, SemanticCacheHit) else str(hit or "")
        if not key or self._redis is None:
            return
        try:
            await self._redis.hincrby(key, "hit_count", 1)
            await self._redis.hset(key, mapping={"last_hit_at": str(_now_ms())})
        except Exception:
            return

    def _classify_response_error(self, exc: Exception) -> str:
        message = str(exc).lower()
        if "unknown command" in message or "unknown command 'ft" in message:
            return "redisearch_missing"
        if "unknown index name" in message or "no such index" in message:
            return "unknown_index"
        if "index" in message and "not found" in message:
            return "index_missing"
        return type(exc).__name__
