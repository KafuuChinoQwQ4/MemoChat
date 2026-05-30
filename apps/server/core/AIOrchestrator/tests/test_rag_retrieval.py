from __future__ import annotations

import asyncio
import unittest

from rag.retrieval import (
    CrossEncoderReranker,
    RetrievalCandidate,
    build_metadata_filter,
    candidate_from_payload,
    collection_matches_filters,
    expand_query_text,
    fuse_candidates,
    metadata_matches_payload,
    rank_lexical_candidates,
    tokenize_text,
)

try:
    from langchain_core.documents import Document
    from rag.chain import RAGChain
except ModuleNotFoundError:
    Document = None
    RAGChain = None


class FakeCollection:
    def __init__(self, name: str):
        self.name = name


class FakeCollectionsResponse:
    def __init__(self, names: list[str]):
        self.collections = [FakeCollection(name) for name in names]


class FakeQdrantClient:
    def __init__(self, *, create_error: Exception | None = None, upsert_error: Exception | None = None):
        self.create_error = create_error
        self.upsert_error = upsert_error
        self.created_collections: list[str] = []
        self.deleted_collections: list[str] = []
        self.upserted_collections: list[str] = []

    def get_collections(self):
        return FakeCollectionsResponse([])

    def create_collection(self, collection_name, vectors_config):
        if self.create_error is not None:
            raise self.create_error
        self.created_collections.append(collection_name)

    def upsert(self, collection_name, points):
        if self.upsert_error is not None:
            raise self.upsert_error
        self.upserted_collections.append(collection_name)

    def delete_collection(self, collection_name):
        self.deleted_collections.append(collection_name)


class FakeEmbedder:
    def get_dimension(self) -> int:
        return 4

    async def embed(self, texts: list[str]) -> list[list[float]]:
        return [[1.0, 0.0, 0.0, 0.0] for _ in texts]


class HybridRagRetrievalTests(unittest.TestCase):
    def test_tokenize_text_preserves_english_terms_and_chinese_terms(self):
        tokens = tokenize_text("MemoChat 知识库 retrieval")

        self.assertIn("memochat", tokens)
        self.assertIn("retrieval", tokens)
        self.assertTrue(any("知" in token or "识" in token for token in tokens))

    def test_metadata_matches_payload_supports_flat_and_nested_metadata(self):
        flat_payload = {
            "page_content": "flat",
            "kb_id": "kb_001",
            "source": "guide.md",
            "file_type": "md",
            "chunk_index": 2,
        }
        nested_payload = {
            "page_content": "nested",
            "metadata": {
                "kb_id": "kb_002",
                "source": "manual.pdf",
                "file_type": "pdf",
                "page": 4,
                "chunk_index": 1,
            },
        }

        self.assertTrue(
            metadata_matches_payload(
                flat_payload,
                {"source": "guide.md", "file_type": "md", "chunk_index_min": 1, "chunk_index_max": 3},
            )
        )
        self.assertTrue(
            metadata_matches_payload(
                nested_payload,
                {"kb_id": "kb_002", "source": "manual.pdf", "file_type": "pdf", "page_min": 3, "page_max": 5},
            )
        )
        self.assertFalse(metadata_matches_payload(nested_payload, {"file_type": "txt"}))

    def test_build_metadata_filter_includes_uid_and_value_clauses(self):
        metadata_filter = build_metadata_filter(
            7,
            {"source": "guide.md", "file_type": "md", "page_min": 2, "page_max": 5},
        )

        self.assertIsNotNone(metadata_filter)
        self.assertGreaterEqual(len(metadata_filter.must), 2)
        self.assertEqual(metadata_filter.must[0].key, "uid")

    def test_collection_matches_filters_uses_requested_kb_ids(self):
        self.assertTrue(collection_matches_filters("user_7_kb_abc", "user_", 7, {"kb_id": "kb_abc"}))
        self.assertFalse(collection_matches_filters("user_7_kb_abc", "user_", 7, {"kb_id": "kb_xyz"}))

    def test_rank_lexical_candidates_prefers_term_overlap(self):
        candidates = [
            candidate_from_payload(
                candidate_id="1",
                payload={"page_content": "MemoChat supports hybrid retrieval and reranking.", "source": "a.md"},
                route="bm25",
            ),
            candidate_from_payload(
                candidate_id="2",
                payload={"page_content": "Unrelated cooking notes.", "source": "b.md"},
                route="bm25",
            ),
        ]

        ranked = rank_lexical_candidates(candidates, "hybrid retrieval reranking")

        self.assertEqual(ranked[0].candidate_id, "1")
        self.assertGreater(ranked[0].lexical_score, ranked[1].lexical_score)
        self.assertEqual(ranked[0].route_ranks["bm25"], 1)

    def test_expand_query_text_adds_bilingual_domain_terms(self):
        expanded = expand_query_text("Prometheus 的本地端口是什么？")

        self.assertIn("ports", expanded)
        self.assertIn("local", expanded)

    def test_rank_lexical_candidates_boosts_authoritative_port_document(self):
        authoritative = candidate_from_payload(
            candidate_id="auth",
            payload={
                "page_content": (
                    "Observability containers keep stable ports: Prometheus 9090, Grafana 3000, Loki 3100, Tempo 3200."
                ),
                "source": "memochat-loadtest-observability.md",
            },
            route="bm25",
        )
        distractor = candidate_from_payload(
            candidate_id="noise",
            payload={
                "page_content": (
                    "Distractor note. Prometheus appears in historical migration notes, "
                    "but this paragraph is not the authoritative answer. It includes "
                    "a near-miss value 9099 next to 9090."
                ),
                "source": "memochat-rag-distractor-28-metrics-stack.md",
            },
            route="bm25",
        )

        ranked = rank_lexical_candidates(
            [distractor, authoritative],
            "Prometheus、Grafana、Loki、Tempo 的本地端口分别是什么？",
        )

        self.assertEqual(ranked[0].candidate_id, "auth")

    def test_heuristic_reranker_penalizes_explicit_distractors(self):
        reranker = CrossEncoderReranker(model_name="fake-model", enabled=True)
        candidates = [
            RetrievalCandidate(
                candidate_id="noise",
                content=(
                    "Distractor note. Prometheus appears in historical migration notes, "
                    "but this paragraph is not the authoritative answer for the active benchmark query."
                ),
                source="memochat-rag-distractor-28-metrics-stack.md",
                fused_score=0.4,
                lexical_score=1.0,
            ),
            RetrievalCandidate(
                candidate_id="auth",
                content="Observability containers keep stable ports: Prometheus 9090, Grafana 3000, Loki 3100, Tempo 3200.",
                source="memochat-loadtest-observability.md",
                fused_score=0.3,
                lexical_score=0.8,
            ),
        ]

        ranked = reranker._heuristic_rerank(
            "Prometheus、Grafana、Loki、Tempo 的本地端口分别是什么？",
            candidates,
            2,
        )

        self.assertEqual(ranked[0].candidate_id, "auth")

    def test_fuse_candidates_merges_dense_and_lexical_routes(self):
        dense = [
            candidate_from_payload(
                candidate_id="doc-1",
                payload={"page_content": "Doc one", "source": "one.md"},
                score=0.9,
                collection="user_1_kb_a",
                route="dense",
            ),
            candidate_from_payload(
                candidate_id="doc-2",
                payload={"page_content": "Doc two", "source": "two.md"},
                score=0.7,
                collection="user_1_kb_a",
                route="dense",
            ),
        ]
        lexical = [
            candidate_from_payload(
                candidate_id="doc-2",
                payload={"page_content": "Doc two", "source": "two.md"},
                score=0.8,
                collection="user_1_kb_a",
                route="bm25",
            ),
            candidate_from_payload(
                candidate_id="doc-3",
                payload={"page_content": "Doc three", "source": "three.md"},
                score=0.6,
                collection="user_1_kb_a",
                route="bm25",
            ),
        ]

        fused = fuse_candidates({"dense": dense, "bm25": lexical}, rrf_k=60)

        self.assertEqual(fused[0].candidate_id, "doc-2")
        self.assertIn("dense", fused[0].recall_routes)
        self.assertIn("bm25", fused[0].recall_routes)
        self.assertGreater(fused[0].fused_score, fused[-1].fused_score)

    def test_cross_encoder_reranker_sorts_by_model_score(self):
        class FakeModel:
            def predict(self, pairs, batch_size=8, show_progress_bar=False):
                self.pairs = list(pairs)
                return [0.1, 4.0]

        reranker = CrossEncoderReranker(model_name="fake-model", enabled=True)
        reranker._model = FakeModel()

        candidates = [
            RetrievalCandidate(candidate_id="low", content="less relevant", source="low.md", fused_score=0.2),
            RetrievalCandidate(candidate_id="high", content="more relevant", source="high.md", fused_score=0.1),
        ]

        ranked = asyncio.run(reranker.rerank("query", candidates, 2))

        self.assertEqual(ranked[0].candidate_id, "high")
        self.assertGreater(ranked[0].rerank_score or 0.0, ranked[1].rerank_score or 0.0)

    @unittest.skipIf(RAGChain is None or Document is None, "rag.chain dependencies are not installed")
    def test_rag_add_documents_propagates_collection_create_failure(self):
        client = FakeQdrantClient(create_error=RuntimeError("too many open files"))
        chain = RAGChain()

        with self.assertRaisesRegex(RuntimeError, "too many open files"):
            asyncio.run(
                chain._add_documents_inner(
                    7,
                    "kb_create_failed",
                    [Document(page_content="content", metadata={"source": "a.md"})],
                    FakeEmbedder(),
                    "user_7_kb_create_failed",
                    client,
                    4,
                )
            )

        self.assertEqual(client.upserted_collections, [])

    @unittest.skipIf(RAGChain is None or Document is None, "rag.chain dependencies are not installed")
    def test_rag_add_documents_rolls_back_new_collection_on_upsert_failure(self):
        client = FakeQdrantClient(upsert_error=RuntimeError("upsert failed"))
        chain = RAGChain()

        with self.assertRaisesRegex(RuntimeError, "upsert failed"):
            asyncio.run(
                chain._add_documents_inner(
                    7,
                    "kb_upsert_failed",
                    [Document(page_content="content", metadata={"source": "a.md"})],
                    FakeEmbedder(),
                    "user_7_kb_upsert_failed",
                    client,
                    4,
                )
            )

        self.assertEqual(client.created_collections, ["user_7_kb_upsert_failed"])
        self.assertEqual(client.deleted_collections, ["user_7_kb_upsert_failed"])


if __name__ == "__main__":
    unittest.main()
