"""
知识库检索工具 — 查询用户私有知识库
"""

from collections.abc import Awaitable, Callable

import structlog
from langchain_core.tools import tool

logger = structlog.get_logger()


KnowledgeSearchProvider = Callable[..., Awaitable[list[dict]]]


class KnowledgeBaseTool:
    """知识库检索工具 — 从 Qdrant 中检索用户上传的文档片段"""

    def __init__(self, search_provider: KnowledgeSearchProvider):
        self._search_provider = search_provider

    async def _search(self, query: str, uid: int = 0, top_k: int = 5) -> str:
        """
        从用户的私有知识库中检索相关文档片段。
        输入搜索问题，返回相关文档片段和来源。
        """
        try:
            results = await self._search_provider(uid=uid, query=query, top_k=top_k)

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
        @tool("knowledge_base_search")
        async def knowledge_base_search(query: str, uid: int = 0, top_k: int = 5) -> str:
            """
            从用户的私有知识库中检索相关文档片段。
            输入搜索问题，返回相关文档片段和来源。
            """
            return await self._search(query, uid=uid, top_k=top_k)

        return knowledge_base_search
