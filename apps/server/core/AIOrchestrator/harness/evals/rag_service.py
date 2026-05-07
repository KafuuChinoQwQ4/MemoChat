from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


def _now_ms() -> int:
    return int(time.time() * 1000)


def _as_list(value: Any) -> list[Any]:
    if value is None:
        return []
    if isinstance(value, list):
        return value
    if isinstance(value, (tuple, set)):
        return list(value)
    if value == "":
        return []
    return [value]


def _normalize_text(value: Any) -> str:
    return str(value or "").strip().lower()


@dataclass
class RagEvalCase:
    case_id: str
    name: str
    description: str = ""
    request: dict[str, Any] = field(default_factory=dict)
    expectations: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "case_id": self.case_id,
            "name": self.name,
            "description": self.description,
            "request": dict(self.request),
            "expectations": dict(self.expectations),
            "metadata": dict(self.metadata),
        }


class RagEvalService:
    def __init__(
        self,
        knowledge_service,
        case_dir: str | Path | None = None,
    ) -> None:
        self._knowledge_service = knowledge_service
        self._case_dir = Path(case_dir) if case_dir is not None else Path(__file__).resolve().parent / "rag_cases"

    def list_cases(self) -> list[RagEvalCase]:
        cases: list[RagEvalCase] = []
        if self._case_dir.exists():
            for path in sorted(self._case_dir.glob("*.json")):
                case = self._load_case_file(path)
                if case is not None:
                    cases.append(case)
        return cases

    def get_case(self, case_id: str) -> RagEvalCase | None:
        for case in self.list_cases():
            if case.case_id == case_id:
                return case
        return None

    async def run_eval(
        self,
        case_id: str = "",
        uid: int = 0,
        metadata_filters: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        case = self.get_case(case_id) if case_id else None
        if case_id and case is None:
            return self._failure_result(
                RagEvalCase(case_id=case_id, name=case_id),
                [f"rag eval case not found: {case_id}"],
            )
        if case is None:
            cases = self.list_cases()
            if not cases:
                return self._failure_result(
                    RagEvalCase(case_id=case_id or "missing", name=case_id or "missing"),
                    ["no rag eval cases found"],
                )
            case = cases[0]

        request = dict(case.request)
        if uid > 0:
            request["uid"] = uid
        request.setdefault("uid", 0)
        request.setdefault("query", "")
        request.setdefault("top_k", 5)
        request.setdefault("metadata_filters", {})
        if metadata_filters:
            merged_filters = dict(request.get("metadata_filters") or {})
            merged_filters.update(metadata_filters)
            request["metadata_filters"] = merged_filters

        query = str(request.get("query") or "").strip()
        if not query:
            return self._failure_result(case, ["query is required"])

        started_at = _now_ms()
        try:
            chunks = await self._knowledge_service.search(
                int(request.get("uid") or 0),
                query,
                int(request.get("top_k") or 5),
                request.get("metadata_filters") or {},
            )
        except Exception as exc:
            return self._failure_result(case, [f"search failed: {type(exc).__name__}: {str(exc)[:300]}"])
        latency_ms = _now_ms() - started_at

        return self.evaluate_results(case, request, chunks, latency_ms)

    async def run_suite(
        self,
        uid: int = 0,
        metadata_filters: dict[str, Any] | None = None,
    ) -> list[dict[str, Any]]:
        return [
            await self.run_eval(case.case_id, uid=uid, metadata_filters=metadata_filters)
            for case in self.list_cases()
        ]

    def evaluate_results(
        self,
        case: RagEvalCase,
        request: dict[str, Any],
        chunks: list[dict[str, Any]],
        latency_ms: int = 0,
    ) -> dict[str, Any]:
        expectations = dict(case.expectations)
        metrics = self._rag_metrics(chunks, expectations, latency_ms)
        failures: list[str] = []

        min_hit_count = int(expectations.get("min_hit_count") or 0)
        if min_hit_count > 0 and metrics["hit_count"] < min_hit_count:
            failures.append(f"hit_count {metrics['hit_count']} below {min_hit_count}")

        min_recall = expectations.get("min_recall_at_k")
        if min_recall is not None and float(metrics["recall_at_k"]) < float(min_recall):
            failures.append(f"recall_at_k {metrics['recall_at_k']:.4f} below {float(min_recall):.4f}")

        min_term_coverage = expectations.get("min_expected_term_coverage")
        if min_term_coverage is not None and float(metrics["expected_term_coverage"]) < float(min_term_coverage):
            failures.append(
                "expected_term_coverage "
                f"{metrics['expected_term_coverage']:.4f} below {float(min_term_coverage):.4f}"
            )

        required_terms = [_normalize_text(term) for term in _as_list(expectations.get("required_terms")) if _normalize_text(term)]
        if required_terms:
            matched_terms = set(metrics["matched_terms"])
            for term in required_terms:
                if term not in matched_terms:
                    failures.append(f"missing required term: {term}")

        max_latency_ms = int(expectations.get("max_latency_ms") or 0)
        if max_latency_ms > 0 and metrics["latency_ms"] > max_latency_ms:
            failures.append(f"latency {metrics['latency_ms']}ms exceeds {max_latency_ms}ms")

        return {
            "case_id": case.case_id,
            "passed": not failures,
            "failures": failures,
            "metrics": metrics,
            "expected": expectations,
            "observed": {
                "uid": int(request.get("uid") or 0),
                "query": str(request.get("query") or ""),
                "top_k": int(request.get("top_k") or 0),
                "metadata_filters": dict(request.get("metadata_filters") or {}),
                "chunks": [self._chunk_observation(chunk) for chunk in chunks],
            },
        }

    def _rag_metrics(
        self,
        chunks: list[dict[str, Any]],
        expectations: dict[str, Any],
        latency_ms: int,
    ) -> dict[str, Any]:
        expected_items = self._expected_items(expectations)
        matched_items = self._matched_items(chunks, expected_items)
        expected_terms = [_normalize_text(term) for term in _as_list(expectations.get("expected_terms")) if _normalize_text(term)]
        haystack = "\n".join(_normalize_text(chunk.get("content")) for chunk in chunks)
        matched_terms = sorted({term for term in expected_terms if term in haystack})
        top_score = max((float(chunk.get("score") or 0.0) for chunk in chunks), default=0.0)

        return {
            "latency_ms": max(int(latency_ms), 0),
            "hit_count": len(chunks),
            "top_score": top_score,
            "expected_items": len(expected_items),
            "matched_items": len(matched_items),
            "recall_at_k": (len(matched_items) / len(expected_items)) if expected_items else 1.0,
            "expected_terms": len(expected_terms),
            "matched_terms": matched_terms,
            "expected_term_coverage": (len(matched_terms) / len(expected_terms)) if expected_terms else 1.0,
            "matched_kb_ids": sorted({str(chunk.get("kb_id") or "") for chunk in chunks if chunk.get("kb_id")}),
            "matched_sources": sorted({str(chunk.get("source") or "") for chunk in chunks if chunk.get("source")}),
        }

    def _expected_items(self, expectations: dict[str, Any]) -> list[dict[str, str]]:
        items: list[dict[str, str]] = []
        for kb_id in _as_list(expectations.get("expected_kb_ids")):
            text = _normalize_text(kb_id)
            if text:
                items.append({"type": "kb_id", "value": text})
        for source in _as_list(expectations.get("expected_sources")):
            text = _normalize_text(source)
            if text:
                items.append({"type": "source", "value": text})
        for item in _as_list(expectations.get("expected_items")):
            if isinstance(item, dict):
                item_type = _normalize_text(item.get("type") or "content")
                value = _normalize_text(item.get("value"))
                if value:
                    items.append({"type": item_type, "value": value})
            else:
                text = _normalize_text(item)
                if text:
                    items.append({"type": "content", "value": text})
        return items

    def _matched_items(self, chunks: list[dict[str, Any]], expected_items: list[dict[str, str]]) -> set[str]:
        matched: set[str] = set()
        for item in expected_items:
            item_key = f"{item['type']}:{item['value']}"
            for chunk in chunks:
                if self._chunk_matches_item(chunk, item):
                    matched.add(item_key)
                    break
        return matched

    def _chunk_matches_item(self, chunk: dict[str, Any], item: dict[str, str]) -> bool:
        item_type = item["type"]
        value = item["value"]
        if item_type == "kb_id":
            return _normalize_text(chunk.get("kb_id")) == value
        if item_type == "source":
            return _normalize_text(chunk.get("source")) == value
        if item_type in {"term", "content"}:
            return value in _normalize_text(chunk.get("content"))

        metadata = chunk.get("metadata")
        if isinstance(metadata, dict):
            return _normalize_text(metadata.get(item_type)) == value
        return False

    def _chunk_observation(self, chunk: dict[str, Any]) -> dict[str, Any]:
        return {
            "kb_id": str(chunk.get("kb_id") or ""),
            "source": str(chunk.get("source") or ""),
            "score": float(chunk.get("score") or 0.0),
            "content_preview": str(chunk.get("content") or "")[:500],
            "routes": list(chunk.get("routes") or []),
            "metadata": dict(chunk.get("metadata") or {}) if isinstance(chunk.get("metadata"), dict) else {},
        }

    def _failure_result(self, case: RagEvalCase, failures: list[str]) -> dict[str, Any]:
        return {
            "case_id": case.case_id,
            "passed": False,
            "failures": failures,
            "metrics": {},
            "expected": dict(case.expectations),
            "observed": {},
        }

    def _load_case_file(self, path: Path) -> RagEvalCase | None:
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            return None
        case_id = str(raw.get("case_id", raw.get("id", path.stem)))
        return RagEvalCase(
            case_id=case_id,
            name=str(raw.get("name", case_id)),
            description=str(raw.get("description", "")),
            request=raw.get("request", {}) if isinstance(raw.get("request", {}), dict) else {},
            expectations=raw.get("expectations", {}) if isinstance(raw.get("expectations", {}), dict) else {},
            metadata=raw.get("metadata", {}) if isinstance(raw.get("metadata", {}), dict) else {},
        )
