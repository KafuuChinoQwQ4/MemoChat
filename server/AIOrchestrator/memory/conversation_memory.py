"""
三层记忆系统 — 短期记忆 + 情景记忆 + 语义记忆
"""
import json
import structlog
from typing import Optional

from langchain_core.messages import BaseMessage, HumanMessage, AIMessage
from llm import LLMManager, LLMMessage

logger = structlog.get_logger()


class MemoryManager:
    """
    三层记忆管理器：
    1. 短期记忆 — 当前会话最近 N 条消息
    2. 情景记忆 — 跨会话重要交互（ai_episodic_memory 表）
    3. 语义记忆 — 用户偏好（ai_semantic_memory 表）
    """

    def __init__(self, session_id: str, uid: int):
        self.session_id = session_id
        self.uid = uid
        self.max_short_term = 20
        self._short_term: list[dict] = []

    async def load_context(self) -> list[LLMMessage]:
        """
        加载三层记忆，构建完整对话上下文。
        """
        from langchain_core.messages import SystemMessage

        messages = []

        semantic = await self._load_semantic()
        if semantic:
            messages.append(SystemMessage(
                content=f"【用户偏好】{json.dumps(semantic, ensure_ascii=False)}"
            ))

        episodic = await self._load_episodic()
        if episodic:
            messages.append(SystemMessage(
                content=f"【相关历史】{episodic}"
            ))

        short_term = await self._load_short_term()
        for msg in short_term:
            role = msg.get("role", "user")
            content = msg.get("content", "")
            if role == "user":
                messages.append(HumanMessage(content=content))
            else:
                messages.append(AIMessage(content=content))

        return messages

    async def _load_short_term(self) -> list[dict]:
        """从 Redis/PostgreSQL 加载最近 N 条消息"""
        try:
            from db.postgres_client import PostgresClient
            pg = PostgresClient()
            rows = await pg.fetchall(
                """
                SELECT role, content FROM ai_message
                WHERE session_id = $1 AND deleted_at IS NULL
                ORDER BY created_at DESC
                LIMIT $2
                """,
                self.session_id, self.max_short_term
            )
            rows.reverse()
            self._short_term = [{"role": r["role"], "content": r["content"]} for r in rows]
            return self._short_term
        except Exception as e:
            logger.warning("memory.short_term.load_failed", session_id=self.session_id, error=str(e))
            return self._short_term

    async def _load_episodic(self) -> str:
        """从 ai_episodic_memory 加载相关情景记忆"""
        try:
            from db.postgres_client import PostgresClient
            pg = PostgresClient()
            rows = await pg.fetchall(
                """
                SELECT summary, importance FROM ai_episodic_memory
                WHERE uid = $1 AND importance > 0.6
                ORDER BY importance DESC, created_at DESC
                LIMIT 5
                """,
                self.uid
            )
            if not rows:
                return ""
            summaries = [f"- {r['summary']}" for r in rows]
            return "\n".join(summaries)
        except Exception as e:
            logger.warning("memory.episodic.load_failed", uid=self.uid, error=str(e))
            return ""

    async def _load_semantic(self) -> dict:
        """从 ai_semantic_memory 加载用户偏好"""
        try:
            from db.postgres_client import PostgresClient
            pg = PostgresClient()
            row = await pg.fetchone(
                "SELECT preferences FROM ai_semantic_memory WHERE uid = $1",
                self.uid
            )
            if row:
                return json.loads(row["preferences"])
            return {}
        except Exception as e:
            logger.warning("memory.semantic.load_failed", uid=self.uid, error=str(e))
            return {}

    async def save_episodic(self, session_id: str, user_msg: str, ai_msg: str):
        """交互结束后，将重要信息存入情景记忆"""
        try:
            from db.postgres_client import PostgresClient
            pg = PostgresClient()
            importance = self._judge_importance(user_msg, ai_msg)

            if importance > 0.5:
                summary = f"用户：{user_msg[:80]} / AI：{ai_msg[:80]}"
                await pg.execute(
                    """
                    INSERT INTO ai_episodic_memory (uid, session_id, summary, entities, importance, created_at)
                    VALUES ($1, $2, $3, '{}', $4, $5)
                    """,
                    self.uid, session_id, summary, importance,
                    int(__import__("time").time() * 1000)
                )

        except Exception as e:
            logger.warning("memory.episodic.save_failed", uid=self.uid, error=str(e))

    async def update_preference(self, key: str, value):
        """更新语义记忆"""
        try:
            from db.postgres_client import PostgresClient
            pg = PostgresClient()
            now = int(__import__("time").time() * 1000)
            await pg.execute(
                """
                INSERT INTO ai_semantic_memory (uid, preferences, updated_at)
                VALUES ($1, jsonb_build_object($2, $3::text), $4)
                ON CONFLICT (uid) DO UPDATE
                SET preferences = jsonb_set(ai_semantic_memory.preferences, $5, to_jsonb($3::text)),
                    updated_at = $4
                """,
                self.uid, key, str(value), now, f"{{{key}}}"
            )
        except Exception as e:
            logger.warning("memory.semantic.update_failed", uid=self.uid, key=key, error=str(e))

    def _judge_importance(self, user_msg: str, ai_msg: str) -> float:
        """粗略判断交互重要性"""
        important_keywords = [
            "重要", "记住", "偏好", "喜欢", "不喜欢", "important",
            "remember", "preference", "喜欢", "不喜欢", "过敏",
        ]
        text = user_msg + ai_msg
        count = sum(1 for kw in important_keywords if kw in text)
        return min(1.0, 0.3 + count * 0.2)
