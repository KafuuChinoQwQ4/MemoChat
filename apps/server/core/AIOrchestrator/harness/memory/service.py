from __future__ import annotations

import json
import re
import time

import structlog

from config import settings
from db.postgres_client import PostgresClient
from harness.contracts import ContextPack, ContextSection, MemorySnapshot
from llm.base import LLMMessage

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


def _estimate_tokens(text: str) -> int:
    return max(len(text) // 4, 1) if text else 0


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

        context_pack = self._build_context_pack(
            uid=uid,
            session_id=session_id,
            chat_history=chat_history,
            episodic=episodic,
            semantic=semantic,
            graph_context=graph_context,
            system_messages=system_messages,
        )

        return MemorySnapshot(
            system_messages=system_messages,
            chat_history=chat_history,
            episodic_summaries=episodic,
            semantic_profile=semantic,
            graph_context=graph_context,
            context_pack=context_pack,
        )

    async def save_after_response(self, uid: int, session_id: str, user_message: str, ai_message: str) -> None:
        await self._save_episodic(uid, session_id, user_message, ai_message)
        await self._upsert_semantic(uid, user_message)
        if self._graph_memory_service is not None:
            await self._graph_memory_service.project_interaction(uid, session_id, user_message, ai_message)

    async def list_visible_memories(self, uid: int) -> list[dict]:
        memories: list[dict] = []
        try:
            pg = PostgresClient()
            semantic_row = await pg.fetchone(
                "SELECT preferences, updated_at FROM ai_semantic_memory WHERE uid = $1",
                uid,
            )
            if semantic_row:
                raw_preferences = semantic_row["preferences"]
                preferences = raw_preferences if isinstance(raw_preferences, dict) else json.loads(raw_preferences)
                for key, value in sorted(preferences.items()):
                    memories.append(
                        {
                            "memory_id": f"semantic:{key}",
                            "type": "semantic",
                            "source": "manual" if str(key).startswith("manual_") else "ai_semantic_memory",
                            "content": str(value),
                            "created_at": 0,
                            "updated_at": int(semantic_row["updated_at"] or 0),
                            "metadata": {"key": key},
                        }
                    )

            episodic_rows = await pg.fetchall(
                """
                SELECT id, summary, entities, importance, created_at
                FROM ai_episodic_memory
                WHERE uid = $1
                ORDER BY created_at DESC
                LIMIT 50
                """,
                uid,
            )
            for row in episodic_rows:
                entities = row["entities"] if isinstance(row["entities"], dict) else {}
                memories.append(
                    {
                        "memory_id": f"episodic:{row['id']}",
                        "type": "episodic",
                        "source": entities.get("source", "ai_episodic_memory"),
                        "content": row["summary"],
                        "created_at": int(row["created_at"] or 0),
                        "updated_at": int(row["created_at"] or 0),
                        "metadata": {"importance": float(row["importance"] or 0.0)},
                    }
                )
        except Exception as exc:
            logger.warning("memory.visible.list_failed", uid=uid, error=str(exc))
        return memories

    async def create_manual_memory(self, uid: int, content: str) -> dict:
        content = content.strip()
        if not content:
            raise ValueError("memory content cannot be empty")
        now = _now_ms()
        try:
            pg = PostgresClient()
            row = await pg.fetchone(
                """
                INSERT INTO ai_episodic_memory
                (uid, session_id, summary, entities, importance, created_at)
                VALUES ($1, '', $2, $3::jsonb, 1.0, $4)
                RETURNING id, summary, entities, importance, created_at
                """,
                uid,
                content[:500],
                json.dumps({"source": "manual"}, ensure_ascii=False),
                now,
            )
            if row:
                return {
                    "memory_id": f"episodic:{row['id']}",
                    "type": "episodic",
                    "source": "manual",
                    "content": row["summary"],
                    "created_at": int(row["created_at"] or now),
                    "updated_at": int(row["created_at"] or now),
                    "metadata": {"importance": float(row["importance"] or 1.0)},
                }
        except Exception as exc:
            logger.warning("memory.manual.create_failed", uid=uid, error=str(exc))
            raise
        raise RuntimeError("manual memory insert returned no row")

    async def delete_visible_memory(self, uid: int, memory_id: str) -> bool:
        memory_id = memory_id.strip()
        if memory_id.startswith("episodic:"):
            raw_id = memory_id.split(":", 1)[1]
            try:
                pg = PostgresClient()
                result = await pg.execute(
                    "DELETE FROM ai_episodic_memory WHERE uid = $1 AND id = $2",
                    uid,
                    int(raw_id),
                )
                return not str(result).endswith(" 0")
            except Exception as exc:
                logger.warning("memory.episodic.delete_failed", uid=uid, memory_id=memory_id, error=str(exc))
                return False

        if memory_id.startswith("semantic:"):
            key = memory_id.split(":", 1)[1]
            existing = await self._load_semantic(uid)
            if key not in existing:
                return False
            existing.pop(key, None)
            try:
                pg = PostgresClient()
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
                return True
            except Exception as exc:
                logger.warning("memory.semantic.delete_failed", uid=uid, memory_id=memory_id, error=str(exc))
                return False

        return False

    def _build_context_pack(
        self,
        uid: int,
        session_id: str,
        chat_history: list[LLMMessage],
        episodic: list[str],
        semantic: dict,
        graph_context: list[dict],
        system_messages: list[LLMMessage],
    ) -> ContextPack:
        sections: list[ContextSection] = []
        if chat_history:
            content = "\n".join(f"{message.role}: {message.content}" for message in chat_history)
            sections.append(
                ContextSection(
                    name="short_term_chat",
                    source="ai_message",
                    content=content,
                    token_count=_estimate_tokens(content),
                    metadata={"message_count": len(chat_history), "session_id": session_id},
                )
            )
        if semantic:
            content = json.dumps(semantic, ensure_ascii=False)
            sections.append(
                ContextSection(
                    name="semantic_profile",
                    source="ai_semantic_memory",
                    content=content,
                    token_count=_estimate_tokens(content),
                    metadata={"keys": sorted(semantic.keys())},
                )
            )
        if episodic:
            content = "\n".join(episodic)
            sections.append(
                ContextSection(
                    name="episodic_summaries",
                    source="ai_episodic_memory",
                    content=content,
                    token_count=_estimate_tokens(content),
                    metadata={"summary_count": len(episodic)},
                )
            )
        if graph_context:
            content = json.dumps(graph_context, ensure_ascii=False)
            sections.append(
                ContextSection(
                    name="graph_context",
                    source="neo4j",
                    content=content,
                    token_count=_estimate_tokens(content),
                    metadata={"item_count": len(graph_context)},
                )
            )
        return ContextPack(
            sections=sections,
            token_budget=4096,
            source_metadata={"uid": uid, "session_id": session_id},
            system_messages=system_messages,
            metadata={"section_count": len(sections)},
        )

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
