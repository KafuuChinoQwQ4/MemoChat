"""
推荐系统 — 好友/群组/兴趣推荐
基于 Neo4j 图数据库的协同过滤推荐
"""
from typing import Optional
import structlog

logger = structlog.get_logger()


class RecommendationEngine:
    """
    Cypher 查询实现三种推荐：
    1. 好友推荐（共同好友、共同兴趣）
    2. 群组推荐（共同话题、共同好友在的群）
    3. 兴趣推荐（高频话题分析）
    """

    def __init__(self, neo4j_client=None):
        if neo4j_client is not None:
            self._neo4j = neo4j_client
        else:
            from .neo4j_client import Neo4jClient

            self._neo4j = Neo4jClient.get_instance()

    def recommend_friends_common_connections(self, uid: int, limit: int = 10) -> list[dict]:
        """基于共同好友推荐"""
        cypher = """
        MATCH (me:User {uid: $uid})-[:KNOWS]->(friend)-[:KNOWS]->(candidate:User)
        WHERE NOT (me)-[:KNOWS]->(candidate)
          AND candidate.uid <> $uid
        WITH candidate, count(friend) as common_friends
        WHERE common_friends >= 2
        RETURN candidate.uid as uid, candidate.name as name, common_friends
        ORDER BY common_friends DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def recommend_friends_common_interests(self, uid: int, limit: int = 10) -> list[dict]:
        """基于共同兴趣推荐"""
        cypher = """
        MATCH (me:User {uid: $uid})-[:INTERESTED_IN]->(t:Topic)<-[:INTERESTED_IN]-(candidate:User)
        WHERE NOT (me)-[:KNOWS]->(candidate)
          AND candidate.uid <> $uid
        WITH candidate, count(t) as common_interests
        WHERE common_interests >= 1
        RETURN candidate.uid as uid, candidate.name as name, common_interests
        ORDER BY common_interests DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def recommend_friends_2nd_degree(self, uid: int, limit: int = 10) -> list[dict]:
        """二级好友推荐（好友的好友的好友）"""
        cypher = """
        MATCH (me:User {uid: $uid})-[:KNOWS*2]->(candidate:User)
        WHERE NOT (me)-[:KNOWS]->(candidate)
          AND candidate.uid <> $uid
        WITH candidate, count(*) as path_count
        RETURN candidate.uid as uid, candidate.name as name, path_count as score
        ORDER BY score DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def recommend_groups(self, uid: int, limit: int = 10) -> list[dict]:
        """推荐用户可能感兴趣的群组"""
        cypher = """
        MATCH (me:User {uid: $uid})-[:INTERESTED_IN]->(t:Topic)<-[:HAS_TOPIC]-(g:Group)
        WHERE NOT (me)-[:IN_GROUP]->(g)
        WITH g, count(t) as topic_match
        WHERE topic_match >= 1
        RETURN g.group_id as group_id, g.name as name, topic_match
        ORDER BY topic_match DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def recommend_groups_via_friends(self, uid: int, limit: int = 10) -> list[dict]:
        """通过好友所在的群组推荐"""
        cypher = """
        MATCH (me:User {uid: $uid})-[:KNOWS]->(friend)-[:IN_GROUP]->(g:Group)
        WHERE NOT (me)-[:IN_GROUP]->(g)
        WITH g, count(friend) as friend_count
        WHERE friend_count >= 1
        RETURN g.group_id as group_id, g.name as name, friend_count
        ORDER BY friend_count DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def recommend_topics(self, uid: int, since_days: int = 30, limit: int = 20) -> list[dict]:
        """推荐热门话题（基于用户聊天历史）"""
        cypher = """
        MATCH (me:User {uid: $uid})<-[:SENT_BY]-(m)-[:ABOUT]->(t:Topic)
        WHERE m.created_at > timestamp() - ($since_days * 86400000)
        WITH t, count(m) as msg_count
        RETURN t.topic_id as topic_id, t.name as name, msg_count
        ORDER BY msg_count DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, since_days=since_days, limit=limit)

    def recommend_trending_topics(self, since_hours: int = 24, limit: int = 20) -> list[dict]:
        """推荐全站热门话题"""
        cypher = """
        MATCH (m)-[:ABOUT]->(t:Topic)
        WHERE m.created_at > timestamp() - ($hours * 3600000)
        WITH t, count(m) as recent_count
        RETURN t.topic_id as topic_id, t.name as name, recent_count
        ORDER BY recent_count DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, hours=since_hours, limit=limit)

    def get_similar_users_by_behavior(self, uid: int, limit: int = 10) -> list[dict]:
        """基于行为相似性推荐（消息交互模式）"""
        cypher = """
        MATCH (me:User {uid: $uid})-[:MESSAGED]->(target:User)
        WITH me, count {(me)-[:MESSAGED]->()} as my_msg_targets,
             target
        MATCH (target)-[:MESSAGED]->(other:User)
        WHERE other <> me
          AND NOT (me)-[:KNOWS]->(other)
          AND other.uid <> $uid
        WITH other, count {(target)-[:MESSAGED]->()} as their_msg_targets,
             target as shared_contact
        WHERE abs(my_msg_targets - their_msg_targets) < 5
        RETURN other.uid as uid, other.name as name, count(shared_contact) as shared_contacts
        ORDER BY shared_contacts DESC
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def recommend_friends_mixed(self, uid: int, limit: int = 10) -> list[dict]:
        """混合推荐好友（综合多种策略）"""
        conn_results = self.recommend_friends_common_connections(uid, limit=limit * 2)
        int_results = self.recommend_friends_common_interests(uid, limit=limit * 2)

        seen: dict[int, dict] = {}
        for r in conn_results:
            uid_val = r.get("uid")
            if uid_val is not None:
                seen[uid_val] = {
                    "uid": uid_val,
                    "name": r.get("name", ""),
                    "score": r.get("common_friends", 0) * 2,
                    "reasons": ["共同好友"],
                }

        for r in int_results:
            uid_val = r.get("uid")
            if uid_val is not None:
                if uid_val in seen:
                    seen[uid_val]["score"] += r.get("common_interests", 0)
                    seen[uid_val]["reasons"].append("共同兴趣")
                else:
                    seen[uid_val] = {
                        "uid": uid_val,
                        "name": r.get("name", ""),
                        "score": r.get("common_interests", 0),
                        "reasons": ["共同兴趣"],
                    }

        results = list(seen.values())
        results.sort(key=lambda x: x["score"], reverse=True)
        return results[:limit]
