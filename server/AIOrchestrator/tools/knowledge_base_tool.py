"""
知识库检索工具 — 查询用户私有知识库
"""
import structlog
from langchain_core.tools import tool

from rag import RAGChain, EmbeddingManager
from config import settings

logger = structlog.get_logger()


class KnowledgeBaseTool:
    """知识库检索工具 — 从 Qdrant 中检索用户上传的文档片段"""

    @tool("knowledge_base_search", description="在用户的私有知识库中检索相关内容。输入搜索问题，返回相关文档片段和来源。适用于用户问及上传过的文档内容。")
    async def knowledge_base_search(self, query: str, uid: int = 0, top_k: int = 5) -> str:
        """
        从用户的私有知识库中检索相关文档片段。
        """
        try:
            rag = RAGChain()
            embed_mgr = EmbeddingManager(settings.embedding)
            results = await rag.retrieve(uid=uid, query=query, top_k=top_k, embedder=embed_mgr)

            if not results:
                return "知识库中没有找到相关内容。"

            formatted = []
            for i, r in enumerate(results, 1):
                source = r.get("source", "unknown")
                content = r.get("content", "")
                score = r.get("score", 0)
                formatted.append(f"[{i}] 来源: {source} (相关度: {score:.2f})\n{content[:300]}")

            return "\n\n".join(formatted)

        except Exception as e:
            logger.error("knowledge_base.error", query=query, error=str(e))
            return f"知识库检索失败: {str(e)}"

    def get_tool(self):
        return self.knowledge_base_search
