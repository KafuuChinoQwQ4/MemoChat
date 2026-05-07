from __future__ import annotations

import json
import re
import time
from typing import Any

import structlog

from config import settings
from db.postgres_client import PostgresClient
from harness.contracts import ContextPack, ContextSection, MemorySnapshot
from llm.base import LLMMessage

logger = structlog.get_logger()

USER_PROFILE_KEY = "user_profile"
_PROFILE_FIELDS: dict[str, str] = {
    "communication_style": "沟通风格",
    "preferred_language": "语言偏好",
    "preferred_response_format": "回答格式",
    "tone": "语气偏好",
    "domain_interests": "关注领域",
    "expertise_level": "专业水平",
    "long_term_goals": "长期目标",
    "constraints": "常用约束",
    "likes": "喜欢",
    "dislikes": "不喜欢",
    "work_context": "工作上下文",
}
_PROFILE_ALIASES: dict[str, str] = {
    "style": "communication_style",
    "response_style": "communication_style",
    "language": "preferred_language",
    "lang": "preferred_language",
    "format": "preferred_response_format",
    "response_format": "preferred_response_format",
    "interests": "domain_interests",
    "topics": "domain_interests",
    "goals": "long_term_goals",
    "goal": "long_term_goals",
    "preference": "likes",
    "preferences": "likes",
    "like": "likes",
    "dislike": "dislikes",
    "avoid": "dislikes",
    "background": "work_context",
    "role": "work_context",
}
_PROFILE_FIELD_LIMIT = 6


def _now_ms() -> int:
    return int(time.time() * 1000)


def _estimate_tokens(text: str) -> int:
    return max(len(text) // 4, 1) if text else 0


def _safe_int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


class MemoryService:
    def __init__(self, graph_memory_service=None, llm_registry=None):
        self._short_term_limit = settings.harness.short_term_limit
        self._short_term_fetch_limit = max(
            int(getattr(settings.harness, "short_term_fetch_limit", self._short_term_limit * 4)),
            self._short_term_limit,
        )
        self._short_term_token_budget = max(
            int(getattr(settings.harness, "short_term_token_budget", 1600)),
            256,
        )
        self._short_term_summary_token_budget = max(
            int(getattr(settings.harness, "short_term_summary_token_budget", 320)),
            64,
        )
        self._episodic_limit = settings.harness.episodic_limit
        self._graph_memory_service = graph_memory_service
        self._llm_registry = llm_registry

    async def load(self, uid: int, session_id: str, include_graph: bool = False) -> MemorySnapshot:
        system_messages: list[LLMMessage] = []
        chat_history, short_term_summary = await self._load_short_term(session_id)
        episodic = await self._load_episodic(uid)
        semantic = await self._load_semantic(uid)
        graph_context = await self._load_graph_context(uid) if include_graph else []

        if short_term_summary:
            system_messages.append(
                LLMMessage(role="system", content=f"【短期摘要】{short_term_summary}")
            )
        semantic_context = self._format_semantic_profile(semantic)
        if semantic_context:
            system_messages.append(
                LLMMessage(role="system", content=f"【用户画像】\n{semantic_context}")
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
            short_term_summary=short_term_summary,
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
        extracted = await self._extract_long_term_memory(user_message, ai_message)
        await self._save_episodic(uid, session_id, extracted)
        await self._upsert_semantic(uid, extracted)
        if self._graph_memory_service is not None:
            await self._graph_memory_service.project_interaction(uid, session_id, user_message, ai_message)

    async def save_semantic_cache_hit(self, uid: int, session_id: str, user_message: str, ai_message: str) -> None:
        extracted = self._fallback_memory_extraction(user_message, ai_message)
        extracted["source"] = "semantic_cache_hit"
        await self._save_episodic(uid, session_id, extracted)
        await self._upsert_semantic(uid, extracted)

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
                profile = preferences.get(USER_PROFILE_KEY)
                if isinstance(profile, dict):
                    for field, label in _PROFILE_FIELDS.items():
                        values = self._profile_values(profile.get(field))
                        if not values:
                            continue
                        memories.append(
                            {
                                "memory_id": f"semantic_profile:{field}",
                                "type": "profile",
                                "source": "ai_user_profile",
                                "content": f"{label}: {'; '.join(values)}",
                                "created_at": 0,
                                "updated_at": int(semantic_row["updated_at"] or 0),
                                "metadata": {"key": field, "label": label},
                            }
                        )

                for key, value in sorted(preferences.items()):
                    if key == USER_PROFILE_KEY:
                        continue
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

        if memory_id.startswith("semantic_profile:"):
            field = self._canonical_profile_field(memory_id.split(":", 1)[1])
            existing = await self._load_semantic(uid)
            profile = existing.get(USER_PROFILE_KEY)
            if not isinstance(profile, dict) or field not in profile:
                return False
            profile.pop(field, None)
            if profile:
                existing[USER_PROFILE_KEY] = profile
            else:
                existing.pop(USER_PROFILE_KEY, None)
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
                logger.warning("memory.profile.delete_failed", uid=uid, memory_id=memory_id, error=str(exc))
                return False

        return False

    def _build_context_pack(
        self,
        uid: int,
        session_id: str,
        chat_history: list[LLMMessage],
        short_term_summary: str,
        episodic: list[str],
        semantic: dict,
        graph_context: list[dict],
        system_messages: list[LLMMessage],
    ) -> ContextPack:
        sections: list[ContextSection] = []
        if short_term_summary:
            sections.append(
                ContextSection(
                    name="short_term_summary",
                    source="ai_message",
                    content=short_term_summary,
                    token_count=_estimate_tokens(short_term_summary),
                    metadata={"session_id": session_id},
                )
            )
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
            content = self._format_semantic_profile(semantic) or json.dumps(semantic, ensure_ascii=False)
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

    async def _load_short_term(self, session_id: str) -> tuple[list[LLMMessage], str]:
        if not session_id:
            return [], ""
        try:
            pg = PostgresClient()
            rows = await pg.fetchall(
                """
                SELECT role, content, created_at
                FROM ai_message
                WHERE session_id = $1 AND deleted_at IS NULL
                ORDER BY created_at DESC
                LIMIT $2
                """,
                session_id,
                self._short_term_fetch_limit,
            )
            rows.reverse()
            window_rows, overflow_rows = self._select_dynamic_window(rows)
            summary = await self._summarize_short_term(overflow_rows)
            return [
                LLMMessage(role="assistant" if row["role"] == "assistant" else "user", content=row["content"])
                for row in window_rows
            ], summary
        except Exception as exc:
            logger.warning("memory.short_term.load_failed", session_id=session_id, error=str(exc))
            return [], ""

    def _select_dynamic_window(self, rows: list[dict]) -> tuple[list[dict], list[dict]]:
        if not rows:
            return [], []

        selected_indices: list[int] = []
        used_tokens = 0
        for index in range(len(rows) - 1, -1, -1):
            row = rows[index]
            content = str(row.get("content", ""))
            role = str(row.get("role", "user"))
            row_tokens = _estimate_tokens(f"{role}: {content}")
            would_exceed_budget = used_tokens + row_tokens > self._short_term_token_budget
            would_exceed_count = len(selected_indices) >= self._short_term_limit
            if selected_indices and (would_exceed_budget or would_exceed_count):
                break
            selected_indices.append(index)
            used_tokens += row_tokens

        selected_indices.reverse()
        first_selected = selected_indices[0] if selected_indices else len(rows)
        return [rows[index] for index in selected_indices], rows[:first_selected]

    async def _summarize_short_term(self, rows: list[dict]) -> str:
        if not rows:
            return ""

        transcript = self._format_transcript(rows, max_chars=12000)
        if self._llm_registry is not None:
            try:
                response = await self._llm_registry.complete(
                    [
                        LLMMessage(
                            role="system",
                            content=(
                                "你是 MemoChat 的短期记忆摘要器。请把较早的对话压缩为一段中文摘要，"
                                "只保留当前回复仍可能需要的目标、约束、决定、偏好和未完成事项。"
                            ),
                        ),
                        LLMMessage(role="user", content=transcript),
                    ],
                    max_tokens=self._short_term_summary_token_budget,
                    temperature=0.1,
                )
                summary = self._clean_summary(response.content)
                if summary:
                    return summary
            except Exception as exc:
                logger.warning("memory.short_term.summary_failed", error=str(exc))

        return self._fallback_short_term_summary(rows)

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

    def _format_semantic_profile(self, semantic: dict[str, Any]) -> str:
        if not isinstance(semantic, dict) or not semantic:
            return ""

        lines: list[str] = []
        profile = semantic.get(USER_PROFILE_KEY)
        if isinstance(profile, dict):
            for field, label in _PROFILE_FIELDS.items():
                values = self._profile_values(profile.get(field))
                if not values:
                    continue
                lines.append(f"- {label}: {'; '.join(values[:_PROFILE_FIELD_LIMIT])}")

        legacy_items: list[str] = []
        for key, value in sorted(semantic.items()):
            if key == USER_PROFILE_KEY:
                continue
            if isinstance(value, (dict, list)):
                value_text = json.dumps(value, ensure_ascii=False)
            else:
                value_text = str(value)
            value_text = value_text.strip()
            if value_text:
                legacy_items.append(f"{key}={value_text[:160]}")
        if legacy_items:
            lines.append(f"- 其他稳定偏好: {'; '.join(legacy_items[:8])}")

        if not lines:
            return ""
        return (
            "以下是按当前用户 uid 隔离的长期画像。用于匹配回答风格、语言、格式、示例和深浅；"
            "当前用户明确指令、事实准确性和安全规则始终优先，不要主动暴露画像内容。\n"
            + "\n".join(lines)
        )

    async def _load_graph_context(self, uid: int) -> list[dict]:
        if not settings.neo4j.enabled or self._graph_memory_service is None:
            return []
        try:
            return await self._graph_memory_service.recall_context(uid)
        except Exception as exc:
            logger.warning("memory.graph.load_failed", uid=uid, error=str(exc))
            return []

    async def _save_episodic(self, uid: int, session_id: str, extracted: dict[str, Any]) -> None:
        episodes = extracted.get("episodic", [])
        if not isinstance(episodes, list):
            return
        try:
            pg = PostgresClient()
            recent_rows = await pg.fetchall(
                """
                SELECT summary
                FROM ai_episodic_memory
                WHERE uid = $1
                ORDER BY created_at DESC
                LIMIT 20
                """,
                uid,
            )
            recent_summaries = {str(row.get("summary", "")).strip() for row in recent_rows}
            now = _now_ms()
            for episode in episodes[:3]:
                if not isinstance(episode, dict):
                    continue
                summary = str(episode.get("summary", "")).strip()[:500]
                if not summary or summary in recent_summaries:
                    continue
                importance = self._safe_importance(episode.get("importance", 0.65))
                if importance <= 0.5:
                    continue
                entities = episode.get("entities", {})
                if not isinstance(entities, dict):
                    entities = {}
                entities.setdefault("source", "llm_extraction" if extracted.get("source") == "llm" else "heuristic")
                await pg.execute(
                    """
                    INSERT INTO ai_episodic_memory
                    (uid, session_id, summary, entities, importance, created_at)
                    VALUES ($1, $2, $3, $4::jsonb, $5, $6)
                    """,
                    uid,
                    session_id,
                    summary,
                    json.dumps(entities, ensure_ascii=False),
                    importance,
                    now,
                )
        except Exception as exc:
            logger.warning("memory.episodic.save_failed", uid=uid, error=str(exc))

    async def _upsert_semantic(self, uid: int, extracted: dict[str, Any]) -> None:
        preferences = extracted.get("semantic", {})
        if not isinstance(preferences, dict):
            preferences = {}
        preferences = {
            str(key).strip()[:64]: str(value).strip()[:500]
            for key, value in preferences.items()
            if str(key).strip() and str(key).strip() != USER_PROFILE_KEY and str(value).strip()
        }
        user_profile = extracted.get(USER_PROFILE_KEY, {})
        if not isinstance(user_profile, dict):
            user_profile = {}
        if not preferences and not user_profile:
            return

        try:
            pg = PostgresClient()
            existing = await self._load_semantic(uid)
            now = _now_ms()
            existing.update(preferences)
            merged_profile = self._merge_user_profile(
                existing.get(USER_PROFILE_KEY),
                user_profile,
                source=str(extracted.get("source") or "unknown"),
                now=now,
            )
            if merged_profile:
                existing[USER_PROFILE_KEY] = merged_profile
            await pg.execute(
                """
                INSERT INTO ai_semantic_memory (uid, preferences, updated_at)
                VALUES ($1, $2::jsonb, $3)
                ON CONFLICT (uid) DO UPDATE
                SET preferences = $2::jsonb, updated_at = $3
                """,
                uid,
                json.dumps(existing, ensure_ascii=False),
                now,
            )
        except Exception as exc:
            logger.warning("memory.semantic.save_failed", uid=uid, error=str(exc))

    async def _extract_long_term_memory(self, user_message: str, ai_message: str) -> dict[str, Any]:
        if self._llm_registry is not None:
            try:
                response = await self._llm_registry.complete(
                    [
                        LLMMessage(
                            role="system",
                            content=(
                                "你是 MemoChat 的长期记忆抽取器。只返回 JSON 对象，不要 Markdown。"
                                "JSON 格式：{\"semantic\":{\"key\":\"value\"},"
                                "\"user_profile\":{\"communication_style\":[],\"preferred_language\":[],"
                                "\"preferred_response_format\":[],\"tone\":[],\"domain_interests\":[],"
                                "\"expertise_level\":[],\"long_term_goals\":[],\"constraints\":[],"
                                "\"likes\":[],\"dislikes\":[],\"work_context\":[]},"
                                "\"episodic\":[{\"summary\":\"...\",\"importance\":0.0,\"entities\":{}}]}。"
                                "semantic 只记录稳定偏好、身份、长期目标、常用约束；"
                                "user_profile 只记录可用于个性化的稳定习惯，例如沟通风格、语言、格式、"
                                "专业背景、关注领域和长期目标；"
                                "episodic 只记录日后有复用价值的事实、决定、承诺或重要事件。"
                                "不要记录密码、密钥、证件号、支付信息，敏感健康/财务/身份信息只有用户明确要求记住时才记录。"
                                "没有可记忆信息时返回空对象和空数组。"
                            ),
                        ),
                        LLMMessage(
                            role="user",
                            content=(
                                "用户消息：\n"
                                f"{user_message[:4000]}\n\n"
                                "AI 回复：\n"
                                f"{ai_message[:4000]}"
                            ),
                        ),
                    ],
                    max_tokens=700,
                    temperature=0.1,
                )
                parsed = self._parse_json_object(response.content)
                normalized = self._normalize_memory_extraction(parsed)
                if normalized["semantic"] or normalized[USER_PROFILE_KEY] or normalized["episodic"]:
                    normalized["source"] = "llm"
                    return normalized
            except Exception as exc:
                logger.warning("memory.long_term.extract_failed", error=str(exc))

        return self._fallback_memory_extraction(user_message, ai_message)

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

    def _fallback_memory_extraction(self, user_message: str, ai_message: str) -> dict[str, Any]:
        preferences = self._extract_preferences(user_message)
        user_profile = self._extract_user_profile_signals(user_message)
        importance = self._judge_importance(user_message, ai_message)
        episodes: list[dict[str, Any]] = []
        if importance > 0.5:
            episodes.append(
                {
                    "summary": f"用户：{user_message[:80]} / AI：{ai_message[:120]}",
                    "importance": importance,
                    "entities": {"source": "heuristic"},
                }
            )
        return {"semantic": preferences, USER_PROFILE_KEY: user_profile, "episodic": episodes, "source": "heuristic"}

    def _normalize_memory_extraction(self, value: dict[str, Any]) -> dict[str, Any]:
        semantic = value.get("semantic", {})
        if not isinstance(semantic, dict):
            semantic = {}
        raw_profile = value.get(USER_PROFILE_KEY, value.get("profile", {}))
        if not isinstance(raw_profile, dict):
            raw_profile = {}
        normalized_semantic = {
            str(key).strip()[:64]: str(item).strip()[:500]
            for key, item in semantic.items()
            if str(key).strip() and str(key).strip() != USER_PROFILE_KEY and str(item).strip()
        }

        raw_episodes = value.get("episodic", [])
        if not isinstance(raw_episodes, list):
            raw_episodes = []
        episodes: list[dict[str, Any]] = []
        for raw_episode in raw_episodes:
            if isinstance(raw_episode, str):
                raw_episode = {"summary": raw_episode}
            if not isinstance(raw_episode, dict):
                continue
            summary = str(raw_episode.get("summary", "")).strip()
            if not summary:
                continue
            entities = raw_episode.get("entities", {})
            if not isinstance(entities, dict):
                entities = {}
            episodes.append(
                {
                    "summary": summary[:500],
                    "importance": self._safe_importance(raw_episode.get("importance", 0.65)),
                    "entities": entities,
                }
            )
        return {
            "semantic": normalized_semantic,
            USER_PROFILE_KEY: self._normalize_user_profile(raw_profile),
            "episodic": episodes[:5],
        }

    def _extract_user_profile_signals(self, user_message: str) -> dict[str, Any]:
        text = (user_message or "").strip()
        lowered = text.lower()
        profile: dict[str, list[dict[str, Any]]] = {}

        def add(field: str, value: str, confidence: float = 0.68) -> None:
            value = value.strip()
            if not value:
                return
            field = self._canonical_profile_field(field)
            profile.setdefault(field, []).append({"value": value[:160], "confidence": confidence})

        if any(keyword in text for keyword in ("简洁", "短一点", "直接", "少废话")) or any(
            keyword in lowered for keyword in ("concise", "brief", "short answer")
        ):
            add("communication_style", "简洁直接", 0.78)
            add("preferred_response_format", "短答案优先", 0.72)
        if any(keyword in text for keyword in ("详细", "展开", "一步一步", "解释清楚", "完整说明")) or any(
            keyword in lowered for keyword in ("detailed", "step by step", "explain")
        ):
            add("communication_style", "详细解释", 0.72)
        if any(keyword in text for keyword in ("表格", "列表", "要点", "步骤")):
            add("preferred_response_format", "结构化要点/步骤", 0.68)

        language_match = re.search(r"(?:以后|默认|请|帮我|回答时|回复时)?(?:都)?(?:用|使用|以)\s*(中文|英文|英语|日文|日语|粤语)\s*(?:回答|回复)?", text)
        if language_match:
            language = language_match.group(1)
            if language == "英语":
                language = "英文"
            if language == "日语":
                language = "日文"
            add("preferred_language", language, 0.82)

        context_match = re.search(
            r"我是([^，。,.；;\n]{1,20}(?:工程师|开发|程序员|学生|产品经理|设计师|运营|老师|架构师|负责人))",
            text,
        )
        if context_match:
            add("work_context", context_match.group(1), 0.74)

        stack_match = re.search(r"(?:我常用|我主要用|技术栈是|正在用)\s*([^，。,.；;\n]{2,60})", text)
        if stack_match:
            add("domain_interests", stack_match.group(1), 0.7)

        goal_match = re.search(r"(?:我的目标是|我想要|我希望)\s*([^。；;\n]{4,80})", text)
        if goal_match and any(keyword in text for keyword in ("长期", "以后", "目标", "希望", "默认")):
            add("long_term_goals", goal_match.group(1), 0.66)

        constraint_match = re.search(r"(?:以后|默认|每次|记住|请记住)\s*([^。；;\n]{4,120})", text)
        if constraint_match:
            add("constraints", constraint_match.group(1), 0.74)

        avoid_match = re.search(r"(?:不要|不喜欢|讨厌)\s*([^。；;\n]{2,80})", text)
        if avoid_match:
            add("dislikes", avoid_match.group(1), 0.72)

        return profile

    def _canonical_profile_field(self, field: str) -> str:
        raw = str(field or "").strip()
        lowered = raw.lower()
        if lowered in _PROFILE_FIELDS:
            return lowered
        if lowered in _PROFILE_ALIASES:
            return _PROFILE_ALIASES[lowered]
        for canonical, label in _PROFILE_FIELDS.items():
            if raw == label:
                return canonical
        return lowered if lowered in _PROFILE_FIELDS else raw[:64]

    def _normalize_user_profile(self, raw_profile: dict[str, Any]) -> dict[str, list[dict[str, Any]]]:
        if not isinstance(raw_profile, dict):
            return {}
        normalized: dict[str, list[dict[str, Any]]] = {}
        for raw_field, raw_value in raw_profile.items():
            field = self._canonical_profile_field(str(raw_field))
            if field not in _PROFILE_FIELDS:
                continue
            items = self._coerce_profile_items(raw_value)
            if items:
                normalized[field] = items[:_PROFILE_FIELD_LIMIT]
        return normalized

    def _merge_user_profile(
        self,
        existing_profile: Any,
        new_profile: dict[str, Any],
        *,
        source: str,
        now: int,
    ) -> dict[str, list[dict[str, Any]]]:
        merged = self._normalize_user_profile(existing_profile if isinstance(existing_profile, dict) else {})
        incoming = self._normalize_user_profile(new_profile)
        if not incoming:
            return merged

        for field, new_items in incoming.items():
            current_items = list(merged.get(field, []))
            by_key = {self._profile_item_key(item.get("value", "")): item for item in current_items}
            for new_item in new_items:
                value_key = self._profile_item_key(new_item.get("value", ""))
                if not value_key:
                    continue
                if value_key in by_key:
                    item = by_key[value_key]
                    item["confidence"] = min(
                        1.0,
                        max(float(item.get("confidence", 0.6)), float(new_item.get("confidence", 0.65))) + 0.05,
                    )
                    item["evidence_count"] = int(item.get("evidence_count", 1)) + int(new_item.get("evidence_count", 1))
                    item["updated_at"] = now
                    item["source"] = source or item.get("source", "unknown")
                    continue

                item = dict(new_item)
                item.setdefault("confidence", 0.65)
                item.setdefault("evidence_count", 1)
                item["updated_at"] = now
                item["source"] = source or item.get("source", "unknown")
                current_items.append(item)
                by_key[value_key] = item

            current_items.sort(
                key=lambda item: (
                    float(item.get("confidence", 0.0)),
                    int(item.get("evidence_count", 0)),
                    int(item.get("updated_at", 0)),
                ),
                reverse=True,
            )
            merged[field] = current_items[:_PROFILE_FIELD_LIMIT]
        return merged

    def _coerce_profile_items(self, raw_value: Any) -> list[dict[str, Any]]:
        if raw_value is None:
            return []
        raw_items: list[Any]
        if isinstance(raw_value, dict):
            if "values" in raw_value and isinstance(raw_value["values"], list):
                raw_items = raw_value["values"]
            elif "value" in raw_value:
                raw_items = [raw_value]
            else:
                raw_items = [value for value in raw_value.values()]
        elif isinstance(raw_value, list):
            raw_items = raw_value
        else:
            raw_items = [raw_value]

        items: list[dict[str, Any]] = []
        seen: set[str] = set()
        for raw_item in raw_items:
            if isinstance(raw_item, dict):
                value = str(raw_item.get("value") or raw_item.get("text") or raw_item.get("name") or "").strip()
                confidence = self._safe_importance(raw_item.get("confidence", 0.65))
                evidence_count = max(_safe_int(raw_item.get("evidence_count", raw_item.get("count", 1)), 1), 1)
                updated_at = _safe_int(raw_item.get("updated_at", 0))
                source = str(raw_item.get("source", "") or "")
            else:
                value = str(raw_item).strip()
                confidence = 0.65
                evidence_count = 1
                updated_at = 0
                source = ""
            value = value[:160]
            key = self._profile_item_key(value)
            if not key or key in seen:
                continue
            seen.add(key)
            item = {
                "value": value,
                "confidence": confidence,
                "evidence_count": evidence_count,
            }
            if updated_at:
                item["updated_at"] = updated_at
            if source:
                item["source"] = source
            items.append(item)
        return items

    def _profile_values(self, raw_value: Any) -> list[str]:
        return [item["value"] for item in self._coerce_profile_items(raw_value) if item.get("value")]

    def _profile_item_key(self, value: str) -> str:
        return re.sub(r"\s+", " ", str(value or "")).strip().lower()

    def _parse_json_object(self, content: str) -> dict[str, Any]:
        text = (content or "").strip()
        if not text:
            return {}
        text = re.sub(r"^```(?:json)?\s*|\s*```$", "", text, flags=re.IGNORECASE | re.DOTALL).strip()
        try:
            value = json.loads(text)
        except Exception:
            match = re.search(r"\{.*\}", text, flags=re.DOTALL)
            if not match:
                return {}
            try:
                value = json.loads(match.group(0))
            except Exception:
                return {}
        return value if isinstance(value, dict) else {}

    def _safe_importance(self, value: Any) -> float:
        try:
            return min(max(float(value), 0.0), 1.0)
        except (TypeError, ValueError):
            return 0.65

    def _format_transcript(self, rows: list[dict], max_chars: int) -> str:
        lines: list[str] = []
        total = 0
        for row in rows:
            role = "AI" if row.get("role") == "assistant" else "用户"
            line = f"{role}: {str(row.get('content', '')).strip()}"
            if total + len(line) > max_chars and lines:
                break
            lines.append(line)
            total += len(line)
        return "\n".join(lines)

    def _fallback_short_term_summary(self, rows: list[dict]) -> str:
        transcript = self._format_transcript(rows[-8:], max_chars=1200)
        if not transcript:
            return ""
        return f"较早对话摘要：{transcript}"

    def _clean_summary(self, content: str) -> str:
        summary = (content or "").strip()
        summary = re.sub(r"^```(?:text|markdown)?\s*|\s*```$", "", summary, flags=re.IGNORECASE | re.DOTALL).strip()
        return summary[:1200]
