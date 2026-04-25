from __future__ import annotations

import json
import re
import time

import structlog

from config import settings
from db.postgres_client import PostgresClient
from harness.contracts import MemorySnapshot
from llm.base import LLMMessage

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


class MemoryService:
    def __init__(self, graph_memory_service=None):
        self._short_term_limit = settings.harness.short_term_limit
        self._episodic_limit = settings.harness.episodic_limit
        self._graph_memory_service = graph_memory_service

    async def load(self, uid: int, session_id: str, include_graph: bool = False) -> MemorySnapshot:
        system_messages: list[LLMMessage] = []
        chat_history = await self._load_short_term(session_id)
        episodic = await self._load_episodic(uid)
        semantic = await self._load_semantic(uid)
        graph_context = await self._load_graph_context(uid) if include_graph else []

        if semantic:
            system_messages.append(
                LLMMessage(role="system", content=f"【用户偏好】{json.dumps(semantic, ensure_ascii=False)}")
            )
        if episodic:
            system_messages.append(
                LLMMessage(role="system", content="【相关历史】\n" + "\n".join(f"- {item}" for item in episodic))
            )
        if graph_context:
            system_messages.append(
                LLMMessage(role="system", content=f"【图谱上下文】{json.dumps(graph_context, ensure_ascii=False)}")
            )

        return MemorySnapshot(
            system_messages=system_messages,
            chat_history=chat_history,
            episodic_summaries=episodic,
            semantic_profile=semantic,
            graph_context=graph_context,
        )

    async def save_after_response(self, uid: int, session_id: str, user_message: str, ai_message: str) -> None:
        await self._save_episodic(uid, session_id, user_message, ai_message)
        await self._upsert_semantic(uid, user_message)
        if self._graph_memory_service is not None:
            await self._graph_memory_service.project_interaction(uid, session_id, user_message, ai_message)

    async def _load_short_term(self, session_id: str) -> list[LLMMessage]:
        if not session_id:
            return []
        try:
            pg = PostgresClient()
            rows = await pg.fetchall(
                """
                SELECT role, content
                FROM ai_message
                WHERE session_id = $1 AND deleted_at IS NULL
                ORDER BY created_at DESC
                LIMIT $2
                """,
                session_id,
                self._short_term_limit,
            )
            rows.reverse()
            return [
                LLMMessage(role="assistant" if row["role"] == "assistant" else "user", content=row["content"])
                for row in rows
            ]
        except Exception as exc:
            logger.warning("memory.short_term.load_failed", session_id=session_id, error=str(exc))
            return []

    async def _load_episodic(self, uid: int) -> list[str]:
        try:
            pg = PostgresClient()
            rows = await pg.fetchall(
                """
                SELECT summary
                FROM ai_episodic_memory
                WHERE uid = $1 AND importance > 0.55
                ORDER BY importance DESC, created_at DESC
                LIMIT $2
                """,
                uid,
                self._episodic_limit,
            )
            return [row["summary"] for row in rows]
        except Exception as exc:
            logger.warning("memory.episodic.load_failed", uid=uid, error=str(exc))
            return []

    async def _load_semantic(self, uid: int) -> dict:
        try:
            pg = PostgresClient()
            row = await pg.fetchone(
                "SELECT preferences FROM ai_semantic_memory WHERE uid = $1",
                uid,
            )
            if not row:
                return {}
            raw_value = row["preferences"]
            if isinstance(raw_value, dict):
                return raw_value
            return json.loads(raw_value)
        except Exception as exc:
            logger.warning("memory.semantic.load_failed", uid=uid, error=str(exc))
            return {}

    async def _load_graph_context(self, uid: int) -> list[dict]:
        if not settings.neo4j.enabled or self._graph_memory_service is None:
            return []
        try:
            return await self._graph_memory_service.recall_context(uid)
        except Exception as exc:
            logger.warning("memory.graph.load_failed", uid=uid, error=str(exc))
            return []

    async def _save_episodic(self, uid: int, session_id: str, user_message: str, ai_message: str) -> None:
        importance = self._judge_importance(user_message, ai_message)
        if importance <= 0.5:
            return

        try:
            summary = f"用户：{user_message[:80]} / AI：{ai_message[:120]}"
            pg = PostgresClient()
            await pg.execute(
                """
                INSERT INTO ai_episodic_memory
                (uid, session_id, summary, entities, importance, created_at)
                VALUES ($1, $2, $3, '{}'::jsonb, $4, $5)
                """,
                uid,
                session_id,
                summary,
                importance,
                _now_ms(),
            )
        except Exception as exc:
            logger.warning("memory.episodic.save_failed", uid=uid, error=str(exc))

    async def _upsert_semantic(self, uid: int, user_message: str) -> None:
        preferences = self._extract_preferences(user_message)
        if not preferences:
            return

        try:
            pg = PostgresClient()
            existing = await self._load_semantic(uid)
            existing.update(preferences)
            await pg.execute(
                """
                INSERT INTO ai_semantic_memory (uid, preferences, updated_at)
                VALUES ($1, $2::jsonb, $3)
                ON CONFLICT (uid) DO UPDATE
                SET preferences = $2::jsonb, updated_at = $3
                """,
                uid,
                json.dumps(existing, ensure_ascii=False),
                _now_ms(),
            )
        except Exception as exc:
            logger.warning("memory.semantic.save_failed", uid=uid, error=str(exc))

    def _judge_importance(self, user_message: str, ai_message: str) -> float:
        important_keywords = ["重要", "记住", "偏好", "喜欢", "不喜欢", "过敏", "remember", "preference"]
        text = f"{user_message} {ai_message}"
        hit_count = sum(1 for keyword in important_keywords if keyword in text)
        return min(1.0, 0.3 + hit_count * 0.2)

    def _extract_preferences(self, user_message: str) -> dict:
        preferences: dict[str, str] = {}
        like_match = re.search(r"(喜欢|偏好)\s*([^\n，。,.]{1,24})", user_message)
        dislike_match = re.search(r"(不喜欢|讨厌)\s*([^\n，。,.]{1,24})", user_message)
        if like_match:
            preferences["likes"] = like_match.group(2).strip()
        if dislike_match:
            preferences["dislikes"] = dislike_match.group(2).strip()
        return preferences
