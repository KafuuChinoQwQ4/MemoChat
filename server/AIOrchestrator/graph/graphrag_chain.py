"""
GraphRAG — 图检索 + 向量检索 混合搜索
"""
from typing import Optional
import structlog
from .neo4j_client import Neo4jClient
from rag.chain import RAGChain
from rag.embedder import EmbeddingManager

logger = structlog.get_logger()


class GraphRAGChain:
    """
    混合检索策略：
    1. Qdrant 向量检索（语义相似性）
    2. Neo4j 图查询（实体关系）
    3. 合并结果注入 LLM
    """

    def __init__(self):
        self._neo4j = Neo4jClient.get_instance()
        self._rag = RAGChain()

    def graph_retrieve(self, uid: int, query: str, top_k: int = 10) -> list[dict]:
        """
        从 Neo4j 检索与用户相关的子图
        """
        cypher = """
        MATCH (u:User {uid: $uid})-[r]-(connected)
        WHERE type(r) IN ['KNOWS', 'MESSAGED', 'IN_GROUP', 'INTERESTED_IN']
        RETURN connected, type(r) as rel_type, r
        LIMIT $top_k
        """
        return self._neo4j.run_query(cypher, uid=uid, top_k=top_k)

    def graph_search_by_entity(self, entity_name: str, depth: int = 2, limit: int = 50) -> list[dict]:
        """
        基于实体名搜索相关子图
        """
        cypher = f"""
        MATCH (start:Entity {{name: $name}})
        CALL {{
            WITH start
            MATCH path = (start)-[r*1..{depth}]-(connected)
            RETURN path, nodes(path) as path_nodes, relationships(path) as path_rels
            LIMIT $limit
        }}
        RETURN start, path_nodes, path_rels
        """
        return self._neo4j.run_query(cypher, name=entity_name, limit=limit)

    def find_related_entities(self, entity_name: str, relation_type: str | None = None, limit: int = 20) -> list[dict]:
        """查找与指定实体相关的其他实体"""
        if relation_type:
            cypher = f"""
            MATCH (e:Entity {{name: $name}})-[r:`{relation_type}`]-(related:Entity)
            RETURN related, type(r) as rel_type, r
            LIMIT $limit
            """
        else:
            cypher = """
            MATCH (e:Entity {name: $name})-[r]-(related:Entity)
            RETURN related, type(r) as rel_type, r
            LIMIT $limit
            """
        return self._neo4j.run_query(cypher, name=entity_name, limit=limit)

    async def hybrid_retrieve(
        self, uid: int, query: str, vector_top_k: int = 5, graph_top_k: int = 10
    ) -> dict:
        """
        混合检索：向量 + 图
        返回 {"vector_results": [...], "graph_results": [...], "combined_context": ""}
        """
        from config import settings

        vector_results = []
        graph_results = []

        # 向量检索
        try:
            embed_mgr = EmbeddingManager(settings.embedding)
            query_vec = await embed_mgr.embed_query(query)
            vector_results = await self._rag.retrieve(uid, query, vector_top_k, embed_mgr)
        except Exception as e:
            logger.error("graphrag.vector.error", error=str(e))

        # 图检索
        try:
            graph_results = self.graph_retrieve(uid, query, graph_top_k)
        except Exception as e:
            logger.error("graphrag.graph.error", error=str(e))

        # 合并上下文
        context_parts = []
        if graph_results:
            graph_text = "\n".join(
                f"【关系: {r.get('rel_type', '')}】{r.get('connected', {})}"
                for r in graph_results
            )
            context_parts.append(f"【社交关系】\n{graph_text}")
        if vector_results:
            vector_text = "\n".join(
                f"[来源: {r.get('source', '')}]\n{r.get('content', '')}" for r in vector_results
            )
            context_parts.append(f"【相关内容】\n{vector_text}")

        return {
            "vector_results": vector_results,
            "graph_results": graph_results,
            "combined_context": "\n\n".join(context_parts),
        }

    async def rag_with_context(self, uid: int, query: str, llm_callable) -> str:
        """
        带 GraphRAG 上下文的问答
        """
        retrieval = await self.hybrid_retrieve(uid, query)
        context = retrieval.get("combined_context", "")

        if not context:
            return await llm_callable(query)

        prompt = f"""基于以下上下文信息回答用户问题。如果上下文中没有相关信息，请说明不知道。

上下文：
{context}

用户问题：{query}

回答："""
        return await llm_callable(prompt)
