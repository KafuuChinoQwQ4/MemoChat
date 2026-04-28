from __future__ import annotations

import re
import time

import structlog

from config import settings
from graph.neo4j_client import Neo4jClient
from graph.recommendation import RecommendationEngine

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


class GraphMemoryService:
    _ZH_STOPWORDS = {
        "我们",
        "你们",
        "这个",
        "那个",
        "一下",
        "可以",
        "需要",
        "就是",
        "如果",
        "因为",
        "所以",
        "然后",
        "已经",
        "继续",
        "项目",
        "工程",
        "功能",
        "接口",
        "后端",
        "前端",
        "一个",
        "一些",
    }
    _EN_STOPWORDS = {
        "the",
        "and",
        "for",
        "with",
        "that",
        "this",
        "from",
        "into",
        "your",
        "have",
        "will",
        "about",
        "agent",
        "project",
    }

    def __init__(self):
        self._enabled = settings.neo4j.enabled
        self._neo4j = Neo4jClient.get_instance() if self._enabled else None
        self._recommendation = RecommendationEngine(self._neo4j) if self._enabled else None

    async def project_interaction(self, uid: int, session_id: str, user_message: str, ai_message: str) -> None:
        if not self._enabled or not self._neo4j:
            return

        topics = self._extract_topics(f"{user_message}\n{ai_message}")
        turn_id = f"{session_id}:{_now_ms()}"
        try:
            await self._neo4j.run_query_async(
                """
                MERGE (u:User {uid: $uid})
                SET u.updated_at = timestamp()
                MERGE (s:AgentSession {session_id: $session_id})
                SET s.uid = $uid,
                    s.updated_at = timestamp()
                MERGE (u)-[:HAS_AGENT_SESSION]->(s)
                MERGE (t:AgentTurn {turn_id: $turn_id})
                SET t.uid = $uid,
                    t.session_id = $session_id,
                    t.user_message = $user_message,
                    t.ai_message = $ai_message,
                    t.created_at = timestamp()
                MERGE (s)-[:HAS_TURN]->(t)
                """,
                uid=uid,
                session_id=session_id,
                turn_id=turn_id,
                user_message=user_message[:2000],
                ai_message=ai_message[:4000],
            )
            if topics:
                await self._neo4j.run_query_async(
                    """
                    MATCH (u:User {uid: $uid})
                    MATCH (t:AgentTurn {turn_id: $turn_id})
                    UNWIND $topics AS topic_name
                    MERGE (topic:Topic {name: topic_name})
                    SET topic.updated_at = timestamp()
                    MERGE (u)-[interest:INTERESTED_IN]->(topic)
                    SET interest.weight = coalesce(interest.weight, 0) + 1,
                        interest.updated_at = timestamp()
                    MERGE (t)-[:ABOUT]->(topic)
                    """,
                    uid=uid,
                    turn_id=turn_id,
                    topics=topics,
                )
        except Exception as exc:
            logger.warning("graph_memory.project_failed", uid=uid, session_id=session_id, error=str(exc))

    async def recall_context(self, uid: int, limit: int = 8) -> list[dict]:
        if not self._enabled or not self._neo4j:
            return []

        try:
            topic_rows = await self._neo4j.run_query_async(
                """
                MATCH (u:User {uid: $uid})-[interest:INTERESTED_IN]->(topic:Topic)
                RETURN topic.name AS topic, interest.weight AS weight
                ORDER BY weight DESC, topic ASC
                LIMIT $limit
                """,
                uid=uid,
                limit=limit,
            )
            turn_rows = await self._neo4j.run_query_async(
                """
                MATCH (:User {uid: $uid})-[:HAS_AGENT_SESSION]->(:AgentSession)-[:HAS_TURN]->(turn:AgentTurn)
                RETURN turn.user_message AS user_message, turn.ai_message AS ai_message, turn.created_at AS created_at
                ORDER BY turn.created_at DESC
                LIMIT $limit
                """,
                uid=uid,
                limit=min(limit, 5),
            )
            payload = []
            if topic_rows:
                payload.append({"kind": "topics", "items": topic_rows})
            if turn_rows:
                payload.append({"kind": "recent_turns", "items": turn_rows})
            return payload
        except Exception as exc:
            logger.warning("graph_memory.recall_failed", uid=uid, error=str(exc))
            return []

    async def describe_graph_state(self, uid: int, limit: int = 5) -> str:
        if not self._enabled or not self._neo4j or not self._recommendation:
            return "Neo4j graph memory is disabled."

        try:
            context = await self.recall_context(uid, limit=limit)
            topic_recommendations = self._recommendation.recommend_topics(uid, since_days=90, limit=limit)
            friend_recommendations = self._recommendation.recommend_friends_mixed(uid, limit=limit)

            sections = []
            if context:
                sections.append(f"graph_context={context}")
            if topic_recommendations:
                sections.append(f"topic_recommendations={topic_recommendations}")
            if friend_recommendations:
                sections.append(f"friend_recommendations={friend_recommendations}")
            return "\n".join(sections) if sections else "No graph context found for this user."
        except Exception as exc:
            logger.warning("graph_memory.describe_failed", uid=uid, error=str(exc))
            return f"Graph recall failed: {exc}"

    def _extract_topics(self, text: str) -> list[str]:
        zh_tokens = re.findall(r"[\u4e00-\u9fff]{2,8}", text)
        en_tokens = re.findall(r"[A-Za-z][A-Za-z0-9_-]{2,24}", text)

        tokens: list[str] = []
        for token in zh_tokens:
            if token in self._ZH_STOPWORDS:
                continue
            tokens.append(token)
        for token in en_tokens:
            lowered = token.lower()
            if lowered in self._EN_STOPWORDS:
                continue
            tokens.append(token)

        deduped: list[str] = []
        seen: set[str] = set()
        for token in tokens:
            normalized = token.lower()
            if normalized in seen:
                continue
            seen.add(normalized)
            deduped.append(token)
            if len(deduped) >= 12:
                break
        return deduped
