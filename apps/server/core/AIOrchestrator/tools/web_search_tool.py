"""
网络搜索工具
"""

import asyncio
import html
import re
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET

import structlog
from langchain_core.tools import tool

logger = structlog.get_logger()


class WebSearchTool:
    """网络搜索工具 — 搜索互联网获取最新信息"""

    async def _search(self, query: str, max_results: int = 5) -> str:
        """
        搜索互联网。
        输入搜索关键词，返回相关结果摘要。
        适用于需要实时信息、新闻、版本状态等场景。
        """
        try:
            result_limit = min(max(int(max_results or 5), 1), 10)
            results = await asyncio.to_thread(self._search_raw_sync, query, result_limit)
            formatted = self._format_results(results)
            if not formatted:
                return "未找到相关结果。"
            return "\n".join(formatted)

        except Exception as e:
            logger.error("web_search.error", query=query, error=str(e))
            return f"搜索失败: {str(e)}"

    def _search_raw_sync(self, query: str, max_results: int) -> list[dict]:
        encoded_query = urllib.parse.urlencode({"format": "rss", "q": query})
        request = urllib.request.Request(
            f"https://www.bing.com/search?{encoded_query}",
            headers={
                "Accept": "application/rss+xml, application/xml;q=0.9, */*;q=0.8",
                "User-Agent": "Mozilla/5.0 MemoChat-AIOrchestrator/1.0",
            },
        )
        with urllib.request.urlopen(request, timeout=15) as response:
            payload = response.read()
        return self._parse_rss(payload, max_results)

    def _parse_rss(self, payload: bytes, max_results: int) -> list[dict]:
        root = ET.fromstring(payload)
        results: list[dict] = []
        for item in root.findall("./channel/item"):
            title = self._node_text(item, "title")
            href = self._node_text(item, "link")
            body = self._clean_text(self._node_text(item, "description"))
            if title or body:
                results.append({"title": title, "href": href, "body": body})
            if len(results) >= max_results:
                break
        return results

    def _node_text(self, node: ET.Element, child_name: str) -> str:
        child = node.find(child_name)
        if child is None or child.text is None:
            return ""
        return self._clean_text(child.text)

    def _clean_text(self, value: str) -> str:
        text = html.unescape(value or "")
        text = re.sub(r"<[^>]+>", " ", text)
        return re.sub(r"\s+", " ", text).strip()

    def _format_results(self, results: list[dict]) -> list[str]:
        formatted: list[str] = []
        for result in results:
            title = result.get("title", "")
            body = result.get("body", "")
            href = result.get("href", "")
            if title and body:
                formatted.append(f"- [{title}]({href}): {body[:200]}")
            elif title:
                formatted.append(f"- {title}")
        return formatted

    def get_tool(self):
        @tool("web_search")
        async def web_search(query: str, max_results: int = 5) -> str:
            """
            搜索互联网。
            输入搜索关键词，返回相关结果摘要。
            适用于需要实时信息、新闻、版本状态等场景。
            """
            return await self._search(query, max_results=max_results)

        return web_search
