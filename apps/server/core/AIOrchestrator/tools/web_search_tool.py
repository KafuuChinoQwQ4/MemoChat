"""LLM-friendly public web search powered by Jina Reader Search."""

import os
from urllib.parse import quote

import httpx
import structlog
from langchain_core.tools import tool

logger = structlog.get_logger()

_JINA_SEARCH_URL = "https://s.jina.ai"
_TIMEOUT_SECONDS = 30.0
_MAX_OUTPUT_CHARS = 24_000


class WebSearchTool:
    """Search and read public pages in a format prepared for language models."""

    def __init__(
        self,
        api_key: str | None = None,
        transport: httpx.AsyncBaseTransport | None = None,
    ) -> None:
        self._api_key = (api_key if api_key is not None else os.getenv("JINA_API_KEY", "")).strip()
        self._transport = transport

    async def _search(self, query: str, max_results: int = 5) -> str:
        clean_query = " ".join(str(query or "").split())
        if not clean_query:
            return "搜索失败: 搜索关键词不能为空。"
        if not self._api_key:
            return "搜索不可用: 未配置 JINA_API_KEY，请为 AI Orchestrator 配置 Jina Reader API 凭据。"

        result_limit = min(max(int(max_results or 5), 1), 10)
        url = f"{_JINA_SEARCH_URL}/{quote(clean_query, safe='')}"
        headers = {
            "Accept": "text/plain",
            "Authorization": f"Bearer {self._api_key}",
        }

        try:
            async with httpx.AsyncClient(
                timeout=_TIMEOUT_SECONDS,
                follow_redirects=True,
                transport=self._transport,
            ) as client:
                response = await client.get(url, params={"count": result_limit}, headers=headers)
                response.raise_for_status()
        except httpx.HTTPStatusError as exc:
            status = exc.response.status_code
            logger.error("web_search.http_error", status=status, query=clean_query)
            if status in {401, 403}:
                return "搜索不可用: JINA_API_KEY 无效、无权限或额度不足，请检查 AI Orchestrator 凭据。"
            if status == 429:
                return "搜索失败: Jina Reader Search 请求过于频繁，请稍后重试。"
            return f"搜索失败: Jina Reader Search 返回 HTTP {status}。"
        except httpx.TimeoutException:
            logger.error("web_search.timeout", query=clean_query)
            return "搜索失败: Jina Reader Search 请求超时。"
        except httpx.HTTPError as exc:
            logger.error("web_search.network_error", query=clean_query, error=type(exc).__name__)
            return "搜索失败: 无法连接 Jina Reader Search。"

        content = response.text.strip()
        if not content:
            return "未找到相关结果。"
        if len(content) > _MAX_OUTPUT_CHARS:
            return f"{content[:_MAX_OUTPUT_CHARS]}\n\n[搜索结果已截断]"
        return content

    def get_tool(self):
        @tool("web_search")
        async def web_search(query: str, max_results: int = 5) -> str:
            """
            搜索并读取互联网内容，返回适合 AI 直接引用和总结的 Markdown。
            适用于最新信息、新闻、版本状态和公开网页资料。
            """
            return await self._search(query, max_results=max_results)

        return web_search
