from __future__ import annotations

import asyncio
import math
import re
from collections import Counter
from dataclasses import dataclass, field
from typing import Any

from qdrant_client.http import models as qdrant_models

try:
    import structlog

    logger = structlog.get_logger()
except Exception:
    import logging

    class _FallbackLogger:
        def warning(self, event: str, **kwargs) -> None:
            logging.getLogger(__name__).warning("%s %s", event, kwargs)

    logger = _FallbackLogger()

_TEXT_RE = re.compile(r"[A-Za-z0-9_]+|[\u4e00-\u9fff]+")
_CJK_RE = re.compile(r"[\u4e00-\u9fff]+")
_JIEBA = None
_JIEBA_READY = False


def _is_cjk_segment(text: str) -> bool:
    return bool(_CJK_RE.fullmatch(text))


def _fallback_cjk_tokens(text: str) -> list[str]:
    if len(text) <= 2:
        return [text]
    tokens = [text[index : index + 2] for index in range(len(text) - 1)]
    tokens.append(text)
    return tokens


def tokenize_text(text: str) -> list[str]:
    if not text:
        return []

    normalized = re.sub(r"\s+", " ", text).lower().strip()
    if not normalized:
        return []

    global _JIEBA, _JIEBA_READY
    if not _JIEBA_READY:
        try:
            import jieba as jieba_module  # type: ignore

            _JIEBA = jieba_module
        except Exception:
            _JIEBA = None
        _JIEBA_READY = True

    tokens: list[str] = []
    for segment in _TEXT_RE.findall(normalized):
        if _is_cjk_segment(segment):
            if _JIEBA is not None:
                tokens.extend(token.strip().lower() for token in _JIEBA.lcut(segment) if token.strip())
            else:
                tokens.extend(_fallback_cjk_tokens(segment))
        else:
            tokens.append(segment)

    return [token for token in tokens if token]


def _stringify_value(value: Any) -> str:
    if value is None:
        return ""
    return str(value).strip().lower()


def _normalize_filter_values(value: Any) -> list[Any]:
    if value is None:
        return []
    if isinstance(value, (list, tuple, set)):
        return [item for item in value if item not in (None, "")]
    if value in (None, ""):
        return []
    return [value]


def _extract_payload_metadata(payload: dict[str, Any]) -> dict[str, Any]:
    metadata: dict[str, Any] = {}
    nested_metadata = payload.get("metadata")
    if isinstance(nested_metadata, dict):
        metadata.update(nested_metadata)

    for key in ("kb_id", "uid", "source", "file_type", "page", "chunk_index", "total_chunks", "content_length"):
        if key in payload and payload[key] not in (None, ""):
            metadata.setdefault(key, payload[key])

    return metadata


def _match_scalar_value(candidate: Any, allowed_values: list[Any]) -> bool:
    if not allowed_values:
        return True
    candidate_text = _stringify_value(candidate)
    if candidate_text == "":
        return False
    normalized_allowed = {_stringify_value(value) for value in allowed_values}
    return candidate_text in normalized_allowed


def _match_range(candidate: Any, minimum: Any = None, maximum: Any = None) -> bool:
    if minimum is None and maximum is None:
        return True
    try:
        value = float(candidate)
    except Exception:
        return False
    if minimum is not None and value < float(minimum):
        return False
    if maximum is not None and value > float(maximum):
        return False
    return True


def _collection_kb_id(collection_name: str, prefix: str, uid: int) -> str:
    expected_prefix = f"{prefix}{uid}_"
    if collection_name.startswith(expected_prefix):
        return collection_name[len(expected_prefix) :]
    return collection_name


@dataclass
class RetrievalCandidate:
    candidate_id: str
    content: str = ""
    source: str = ""
    kb_id: str = ""
    collection: str = ""
    metadata: dict[str, Any] = field(default_factory=dict)
    dense_score: float = 0.0
    lexical_score: float = 0.0
    fused_score: float = 0.0
    rerank_score: float | None = None
    recall_routes: set[str] = field(default_factory=set)
    route_ranks: dict[str, int] = field(default_factory=dict)

    def merge(self, other: "RetrievalCandidate") -> None:
        if not self.content:
            self.content = other.content
        if not self.source:
            self.source = other.source
        if not self.kb_id:
            self.kb_id = other.kb_id
        if not self.collection:
            self.collection = other.collection
        self.metadata = _merge_metadata(self.metadata, other.metadata)
        self.dense_score = max(self.dense_score, other.dense_score)
        self.lexical_score = max(self.lexical_score, other.lexical_score)
        self.fused_score = max(self.fused_score, other.fused_score)
        if other.rerank_score is not None:
            self.rerank_score = max(self.rerank_score or 0.0, other.rerank_score)
        self.recall_routes.update(other.recall_routes)
        for route, rank in other.route_ranks.items():
            current = self.route_ranks.get(route)
            if current is None or rank < current:
                self.route_ranks[route] = rank

    @property
    def final_score(self) -> float:
        if self.rerank_score is not None:
            return float(self.rerank_score)
        if self.fused_score:
            return float(self.fused_score)
        return max(float(self.dense_score), float(self.lexical_score))

    def to_dict(self) -> dict[str, Any]:
        return {
            "content": self.content,
            "score": self.final_score,
            "source": self.source,
            "kb_id": self.kb_id,
            "dense_score": float(self.dense_score),
            "bm25_score": float(self.lexical_score),
            "fused_score": float(self.fused_score),
            "rerank_score": None if self.rerank_score is None else float(self.rerank_score),
            "routes": sorted(self.recall_routes),
            "metadata": dict(self.metadata),
        }


def _merge_metadata(base: dict[str, Any], other: dict[str, Any]) -> dict[str, Any]:
    merged = dict(base)
    for key, value in other.items():
        if key not in merged or merged[key] in (None, "", [], {}):
            merged[key] = value
    return merged


def _candidate_key(candidate: RetrievalCandidate) -> str:
    if candidate.candidate_id:
        return candidate.candidate_id
    if candidate.collection:
        return f"{candidate.collection}:{candidate.kb_id}:{candidate.metadata.get('chunk_index', '')}"
    return f"{candidate.kb_id}:{candidate.source}:{candidate.metadata.get('chunk_index', '')}:{candidate.content[:48]}"


def candidate_from_payload(
    *,
    candidate_id: Any,
    payload: dict[str, Any] | None,
    score: float = 0.0,
    collection: str = "",
    route: str = "",
) -> RetrievalCandidate:
    payload = dict(payload or {})
    metadata = _extract_payload_metadata(payload)
    kb_id = _stringify_value(payload.get("kb_id") or metadata.get("kb_id"))
    source = payload.get("source") or metadata.get("source") or collection
    candidate = RetrievalCandidate(
        candidate_id=_stringify_value(candidate_id) or _candidate_key(
            RetrievalCandidate(candidate_id="", content=str(payload.get("page_content", "")), source=str(source), kb_id=str(kb_id), collection=collection, metadata=metadata)
        ),
        content=str(payload.get("page_content", "")),
        source=str(source),
        kb_id=str(kb_id),
        collection=collection,
        metadata=metadata,
        dense_score=float(score) if route == "dense" else 0.0,
        lexical_score=float(score) if route == "bm25" else 0.0,
        recall_routes={route} if route else set(),
    )
    return candidate


def metadata_matches_payload(payload: dict[str, Any] | None, metadata_filters: dict[str, Any] | None) -> bool:
    if not metadata_filters:
        return True
    payload = dict(payload or {})
    metadata = _extract_payload_metadata(payload)

    kb_ids = _normalize_filter_values(metadata_filters.get("kb_ids") or metadata_filters.get("kb_id"))
    if kb_ids and not _match_scalar_value(metadata.get("kb_id"), kb_ids):
        return False

    sources = _normalize_filter_values(metadata_filters.get("sources") or metadata_filters.get("source"))
    if sources and not _match_scalar_value(metadata.get("source"), sources):
        return False

    file_types = _normalize_filter_values(metadata_filters.get("file_types") or metadata_filters.get("file_type"))
    if file_types and not _match_scalar_value(metadata.get("file_type"), file_types):
        return False

    if not _match_range(
        metadata.get("page"),
        metadata_filters.get("page_min"),
        metadata_filters.get("page_max"),
    ):
        return False

    if not _match_range(
        metadata.get("chunk_index"),
        metadata_filters.get("chunk_index_min"),
        metadata_filters.get("chunk_index_max"),
    ):
        return False

    return True


def collection_matches_filters(collection_name: str, prefix: str, uid: int, metadata_filters: dict[str, Any] | None) -> bool:
    if not metadata_filters:
        return True
    kb_ids = _normalize_filter_values(metadata_filters.get("kb_ids") or metadata_filters.get("kb_id"))
    if not kb_ids:
        return True
    collection_kb_id = _collection_kb_id(collection_name, prefix, uid)
    return _match_scalar_value(collection_kb_id, kb_ids)


def build_metadata_filter(uid: int, metadata_filters: dict[str, Any] | None) -> qdrant_models.Filter | None:
    if not metadata_filters:
        return None

    must: list[Any] = [
        qdrant_models.FieldCondition(key="uid", match=qdrant_models.MatchValue(value=uid))
    ]

    kb_ids = _normalize_filter_values(metadata_filters.get("kb_ids") or metadata_filters.get("kb_id"))
    if kb_ids:
        should = [
            qdrant_models.FieldCondition(
                key="kb_id",
                match=qdrant_models.MatchValue(value=kb_id),
            )
            for kb_id in kb_ids
        ]
        should.append(
            qdrant_models.FieldCondition(
                key="metadata.kb_id",
                match=qdrant_models.MatchAny(any=kb_ids) if len(kb_ids) > 1 else qdrant_models.MatchValue(value=kb_ids[0]),
            )
        )
        must.append(qdrant_models.Filter(should=should))

    sources = _normalize_filter_values(metadata_filters.get("sources") or metadata_filters.get("source"))
    if sources:
        should = [
            qdrant_models.FieldCondition(
                key=key,
                match=qdrant_models.MatchAny(any=sources) if len(sources) > 1 else qdrant_models.MatchValue(value=sources[0]),
            )
            for key in ("source", "metadata.source")
        ]
        must.append(qdrant_models.Filter(should=should))

    file_types = _normalize_filter_values(metadata_filters.get("file_types") or metadata_filters.get("file_type"))
    if file_types:
        should = [
            qdrant_models.FieldCondition(
                key=key,
                match=qdrant_models.MatchAny(any=file_types) if len(file_types) > 1 else qdrant_models.MatchValue(value=file_types[0]),
            )
            for key in ("file_type", "metadata.file_type")
        ]
        must.append(qdrant_models.Filter(should=should))

    if metadata_filters.get("page_min") is not None or metadata_filters.get("page_max") is not None:
        page_should = [
            qdrant_models.FieldCondition(
                key=key,
                range=qdrant_models.Range(
                    gte=metadata_filters.get("page_min"),
                    lte=metadata_filters.get("page_max"),
                ),
            )
            for key in ("page", "metadata.page")
        ]
        must.append(qdrant_models.Filter(should=page_should))

    if metadata_filters.get("chunk_index_min") is not None or metadata_filters.get("chunk_index_max") is not None:
        chunk_should = [
            qdrant_models.FieldCondition(
                key=key,
                range=qdrant_models.Range(
                    gte=metadata_filters.get("chunk_index_min"),
                    lte=metadata_filters.get("chunk_index_max"),
                ),
            )
            for key in ("chunk_index", "metadata.chunk_index")
        ]
        must.append(qdrant_models.Filter(should=chunk_should))

    if len(must) == 1:
        return qdrant_models.Filter(must=must)
    return qdrant_models.Filter(must=must)


def tokenize_for_bm25(text: str) -> list[str]:
    return tokenize_text(text)


def _normalize_scores(scores: list[float]) -> list[float]:
    max_score = max(scores) if scores else 0.0
    if max_score <= 0.0:
        return scores
    return [score / max_score for score in scores]


def _bm25_scores(tokenized_docs: list[list[str]], tokenized_query: list[str]) -> list[float]:
    if not tokenized_docs or not tokenized_query:
        return [0.0 for _ in tokenized_docs]

    try:
        from rank_bm25 import BM25Okapi

        raw_scores = [float(score) for score in BM25Okapi(tokenized_docs).get_scores(tokenized_query)]
        if any(score > 0.0 for score in raw_scores) and len({round(score, 8) for score in raw_scores}) > 1:
            return _normalize_scores(raw_scores)
    except Exception:
        pass

    k1 = 1.5
    b = 0.75
    doc_count = len(tokenized_docs)
    doc_lengths = [len(doc) for doc in tokenized_docs]
    avg_doc_length = sum(doc_lengths) / doc_count if doc_count else 0.0
    term_frequencies = [Counter(doc) for doc in tokenized_docs]

    document_frequency: Counter[str] = Counter()
    for doc in tokenized_docs:
        document_frequency.update(set(doc))

    scores: list[float] = []
    for doc_index, doc_tf in enumerate(term_frequencies):
        doc_length = doc_lengths[doc_index] or 1
        score = 0.0
        for term in tokenized_query:
            freq = doc_tf.get(term, 0)
            if not freq:
                continue
            idf = math.log(1.0 + ((doc_count - document_frequency[term] + 0.5) / (document_frequency[term] + 0.5)))
            numerator = freq * (k1 + 1.0)
            denominator = freq + k1 * (1.0 - b + b * (doc_length / avg_doc_length if avg_doc_length else 0.0))
            score += idf * (numerator / denominator)
        scores.append(score)

    return _normalize_scores(scores)


def rank_lexical_candidates(candidates: list[RetrievalCandidate], query: str) -> list[RetrievalCandidate]:
    if not candidates:
        return []

    query_tokens = tokenize_for_bm25(query)
    if not query_tokens:
        return []

    tokenized_docs = [tokenize_for_bm25(candidate.content) for candidate in candidates]
    scores = _bm25_scores(tokenized_docs, query_tokens)

    ranked: list[RetrievalCandidate] = []
    for index, candidate in enumerate(candidates):
        lexical_candidate = RetrievalCandidate(
            candidate_id=candidate.candidate_id,
            content=candidate.content,
            source=candidate.source,
            kb_id=candidate.kb_id,
            collection=candidate.collection,
            metadata=dict(candidate.metadata),
            lexical_score=float(scores[index]),
            recall_routes=set(candidate.recall_routes) | {"bm25"},
        )
        ranked.append(lexical_candidate)

    ranked.sort(key=lambda item: (item.lexical_score, len(item.content)), reverse=True)
    for rank, candidate in enumerate(ranked, start=1):
        candidate.route_ranks["bm25"] = rank
    return ranked


def fuse_candidates(
    route_candidates: dict[str, list[RetrievalCandidate]],
    *,
    route_weights: dict[str, float] | None = None,
    rrf_k: int = 60,
) -> list[RetrievalCandidate]:
    route_weights = route_weights or {"dense": 1.0, "bm25": 1.0}
    merged: dict[str, RetrievalCandidate] = {}

    for route, candidates in route_candidates.items():
        route_weight = route_weights.get(route, 1.0)
        for rank, candidate in enumerate(candidates, start=1):
            key = _candidate_key(candidate)
            existing = merged.get(key)
            if existing is None:
                existing = RetrievalCandidate(
                    candidate_id=candidate.candidate_id,
                    content=candidate.content,
                    source=candidate.source,
                    kb_id=candidate.kb_id,
                    collection=candidate.collection,
                    metadata=dict(candidate.metadata),
                    dense_score=candidate.dense_score,
                    lexical_score=candidate.lexical_score,
                    fused_score=0.0,
                    rerank_score=candidate.rerank_score,
                    recall_routes=set(candidate.recall_routes),
                    route_ranks=dict(candidate.route_ranks),
                )
                merged[key] = existing
            else:
                existing.merge(candidate)

            existing.recall_routes.add(route)
            existing.route_ranks[route] = rank if route not in existing.route_ranks else min(existing.route_ranks[route], rank)
            if route == "dense":
                existing.dense_score = max(existing.dense_score, candidate.dense_score)
            elif route == "bm25":
                existing.lexical_score = max(existing.lexical_score, candidate.lexical_score)
            existing.fused_score += route_weight / (rrf_k + rank)

    fused = list(merged.values())
    fused.sort(key=lambda item: (item.fused_score, item.dense_score, item.lexical_score), reverse=True)
    return fused


class CrossEncoderReranker:
    def __init__(
        self,
        *,
        model_name: str,
        enabled: bool = True,
        batch_size: int = 8,
        device: str | None = None,
    ) -> None:
        self.model_name = model_name
        self.enabled = enabled
        self.batch_size = batch_size
        self.device = device
        self._model = None
        self._load_error: str = ""

    def _load_model(self):
        if self._model is not None or self._load_error:
            return self._model
        try:
            from sentence_transformers import CrossEncoder
        except Exception as exc:
            self._load_error = str(exc)
            logger.warning("rag.rerank.import_failed", model=self.model_name, error=str(exc))
            return None

        try:
            self._model = CrossEncoder(self.model_name, device=self.device)
        except Exception as exc:
            self._load_error = str(exc)
            logger.warning("rag.rerank.model_load_failed", model=self.model_name, error=str(exc))
            return None
        return self._model

    async def rerank(
        self,
        query: str,
        candidates: list[RetrievalCandidate],
        top_k: int,
    ) -> list[RetrievalCandidate]:
        if not self.enabled or not candidates or top_k <= 0:
            return candidates[:top_k]

        model = self._load_model()
        if model is None:
            return candidates[:top_k]

        pairs = [(query, candidate.content) for candidate in candidates]
        try:
            raw_scores = await asyncio.to_thread(
                lambda: model.predict(
                    pairs,
                    batch_size=self.batch_size,
                    show_progress_bar=False,
                )
            )
        except Exception as exc:
            logger.warning("rag.rerank.failed", model=self.model_name, error=str(exc))
            return candidates[:top_k]

        try:
            scores = [float(score) for score in raw_scores]
        except TypeError:
            scores = [float(raw_scores)]

        for candidate, raw_score in zip(candidates, scores):
            candidate.rerank_score = _normalize_rerank_score(raw_score)

        ranked = sorted(
            candidates,
            key=lambda item: (
                item.rerank_score if item.rerank_score is not None else -1.0,
                item.fused_score,
                item.dense_score,
                item.lexical_score,
            ),
            reverse=True,
        )
        return ranked[:top_k]


def _normalize_rerank_score(score: float) -> float:
    if 0.0 <= score <= 1.0:
        return score
    if score >= 0.0:
        try:
            return 1.0 / (1.0 + math.exp(-score))
        except OverflowError:
            return 1.0
    try:
        return 1.0 / (1.0 + math.exp(-score))
    except OverflowError:
        return 0.0
