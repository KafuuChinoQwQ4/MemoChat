"""
图谱构建器
从聊天内容中提取实体（NER）和关系，构建知识图谱
"""
from typing import Optional
import structlog
from .neo4j_client import Neo4jClient

logger = structlog.get_logger()


class GraphBuilder:
    """
    1. 构建 User 节点（已存在用户）
    2. 从消息中提取 Entity 节点
    3. 创建 KNOWS / MESSAGED / IN_GROUP 等关系
    4. 维护消息嵌入向量
    """

    def __init__(self, neo4j_client: Optional[Neo4jClient] = None):
        self._neo4j = neo4j_client or Neo4jClient.get_instance()

    def ensure_user_node(self, uid: int, name: str = "", interests: list[str] | None = None) -> None:
        """确保用户节点存在"""
        cypher = """
        MERGE (u:User {uid: $uid})
        SET u.name = $name,
            u.interests = $interests,
            u.updated_at = timestamp()
        """
        self._neo4j.run_query(cypher, uid=uid, name=name, interests=interests or [])

    def merge_entity(self, entity_name: str, entity_type: str, embedding: list[float] | None = None) -> None:
        """合并实体节点"""
        cypher = """
        MERGE (e:Entity {name: $name, type: $type})
        SET e.embedding = $embedding,
            e.updated_at = timestamp()
        """
        self._neo4j.run_query(cypher, name=entity_name, type=entity_type, embedding=embedding)

    def merge_topic(self, topic_name: str, topic_id: str | None = None) -> None:
        """合并话题节点"""
        cypher = """
        MERGE (t:Topic {name: $name})
        SET t.topic_id = coalesce($topic_id, t.topic_id),
            t.updated_at = timestamp()
        """
        self._neo4j.run_query(cypher, name=topic_name, topic_id=topic_id)

    def merge_interest_relation(self, uid: int, topic_name: str) -> None:
        """创建用户-兴趣关系"""
        cypher = """
        MATCH (u:User {uid: $uid})
        MERGE (t:Topic {name: $topic_name})
        MERGE (u)-[r:INTERESTED_IN]->(t)
        SET r.weight = coalesce(r.weight, 0) + 1,
            r.updated_at = timestamp()
        """
        self._neo4j.run_query(cypher, uid=uid, topic_name=topic_name)

    def merge_friend_relation(self, uid1: int, uid2: int) -> None:
        """创建好友关系"""
        cypher = """
        MATCH (u1:User {uid: $uid1}), (u2:User {uid: $uid2})
        MERGE (u1)-[:KNOWS {since: timestamp()}]->(u2)
        """
        self._neo4j.run_query(cypher, uid1=uid1, uid2=uid2)

    def merge_message_relation(self, uid_from: int, uid_to: int, msg_content: str = "") -> None:
        """记录用户间消息交互"""
        cypher = """
        MATCH (u1:User {uid: $uid_from}), (u2:User {uid: $uid_to})
        MERGE (u1)-[r:MESSAGED]->(u2)
        SET r.count = coalesce(r.count, 0) + 1,
            r.last_time = timestamp(),
            r.last_msg = $last_msg
        """
        self._neo4j.run_query(cypher, uid_from=uid_from, uid_to=uid_to, last_msg=msg_content[:200])

    def merge_message_topic_relation(
        self, uid: int, msg_content: str, topic_name: str, created_at: int | None = None
    ) -> None:
        """记录消息与话题的关系"""
        cypher = """
        MATCH (u:User {uid: $uid})
        MERGE (m:Message {content: $content, uid: $uid})
        SET m.created_at = coalesce($created_at, timestamp())
        MERGE (t:Topic {name: $topic_name})
        MERGE (m)-[:ABOUT]->(t)
        MERGE (m)<-[:SENT_BY]-(u)
        """
        self._neo4j.run_query(
            cypher,
            uid=uid,
            content=msg_content[:500],
            topic_name=topic_name,
            created_at=created_at or None,
        )

    def merge_group_membership(self, uid: int, group_id: int, group_name: str = "") -> None:
        """记录用户群组关系"""
        cypher = """
        MATCH (u:User {uid: $uid})
        MERGE (g:Group {group_id: $group_id})
        SET g.name = $group_name
        MERGE (u)-[r:IN_GROUP]->(g)
        SET r.joined_at = timestamp()
        """
        self._neo4j.run_query(cypher, uid=uid, group_id=group_id, group_name=group_name)

    def merge_entity_relation(
        self, entity1_name: str, entity1_type: str, relation_type: str, entity2_name: str, entity2_type: str
    ) -> None:
        """创建实体间关系"""
        cypher = """
        MERGE (e1:Entity {name: $name1, type: $type1})
        MERGE (e2:Entity {name: $name2, type: $type2})
        MERGE (e1)-[r:`$rel_type`]->(e2)
        SET r.updated_at = timestamp()
        """
        self._neo4j.run_query(
            cypher,
            name1=entity1_name,
            type1=entity1_type,
            rel_type=relation_type,
            name2=entity2_name,
            type2=entity2_type,
        )

    def get_user_neighbors(self, uid: int, relation_types: list[str] | None = None, limit: int = 50) -> list[dict]:
        """获取用户的邻居节点"""
        if relation_types is None:
            relation_types = ["KNOWS", "MESSAGED", "IN_GROUP", "INTERESTED_IN"]

        rel_pattern = "|".join(relation_types)
        cypher = f"""
        MATCH (u:User {{uid: $uid}})-[r:{rel_pattern}]-(connected)
        RETURN connected, type(r) as rel_type, r
        LIMIT $limit
        """
        return self._neo4j.run_query(cypher, uid=uid, limit=limit)

    def delete_user_node(self, uid: int) -> None:
        """删除用户节点及其所有关系"""
        cypher = """
        MATCH (u:User {uid: $uid})
        DETACH DELETE u
        """
        self._neo4j.run_query(cypher, uid=uid)
        logger.info("neo4j.user_deleted", uid=uid)

    def build_full_text_index(self) -> None:
        """创建全文索引"""
        indexes = [
            "CREATE INDEX user_uid IF NOT EXISTS FOR (u:User) ON (u.uid)",
            "CREATE INDEX entity_name IF NOT EXISTS FOR (e:Entity) ON (e.name)",
            "CREATE INDEX group_gid IF NOT EXISTS FOR (g:Group) ON (g.group_id)",
            "CREATE INDEX topic_name IF NOT EXISTS FOR (t:Topic) ON (t.name)",
        ]
        for idx in indexes:
            try:
                self._neo4j.run_query(idx)
                logger.info("neo4j.index.created", index=idx[:60])
            except Exception as e:
                logger.warning("neo4j.index.skip", index=idx[:50], error=str(e))
