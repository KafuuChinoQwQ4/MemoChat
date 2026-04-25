"""
DuckDuckGo 搜索工具
"""
import structlog
from langchain_core.tools import tool
from duckduckgo_search import AsyncDDGS

logger = structlog.get_logger()


class DuckDuckGoSearchTool:
    """DuckDuckGo 搜索工具 — 搜索互联网获取最新信息"""

    @tool("duckduckgo_search")
    async def duckduckgo_search(self, query: str) -> str:
        """
        使用 DuckDuckGo 搜索互联网。
        输入搜索关键词，返回相关结果摘要。
        适用于需要实时信息、新闻、天气预报等场景。
        """
        try:
            async with AsyncDDGS() as ddgs:
                results = []
                async for r in ddgs.atext(query, max_results=5):
                    title = r.get("title", "")
                    body = r.get("body", "")
                    href = r.get("href", "")
                    if title and body:
                        results.append(f"- [{title}]({href}): {body[:200]}")
                    elif title:
                        results.append(f"- {title}")

                if not results:
                    return "未找到相关结果。"

                return "\n".join(results)

        except Exception as e:
            logger.error("duckduckgo.error", query=query, error=str(e))
            return f"搜索失败: {str(e)}"

    def get_tool(self):
        return self.duckduckgo_search
