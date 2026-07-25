import unittest
from urllib.parse import unquote

import httpx
from tools.web_search_tool import WebSearchTool


class WebSearchToolTests(unittest.IsolatedAsyncioTestCase):
    async def test_missing_jina_key_returns_actionable_error_without_request(self):
        requests = []

        def handler(request: httpx.Request) -> httpx.Response:
            requests.append(request)
            return httpx.Response(500)

        result = await WebSearchTool(
            api_key="",
            transport=httpx.MockTransport(handler),
        )._search("MemoChat")

        self.assertEqual(requests, [])
        self.assertIn("JINA_API_KEY", result)

    async def test_search_uses_jina_reader_with_encoded_query_and_bounded_count(self):
        requests = []
        markdown = "Title: MemoChat\nURL Source: https://example.com\nMarkdown Content: result"

        def handler(request: httpx.Request) -> httpx.Response:
            requests.append(request)
            return httpx.Response(200, text=markdown, request=request)

        result = await WebSearchTool(
            api_key="jina_test_key",
            transport=httpx.MockTransport(handler),
        )._search("MemoChat AI / search?", max_results=99)

        self.assertEqual(result, markdown)
        self.assertEqual(len(requests), 1)
        request = requests[0]
        self.assertEqual(request.headers["Authorization"], "Bearer jina_test_key")
        self.assertEqual(request.headers["Accept"], "text/plain")
        self.assertEqual(request.url.params["count"], "10")
        self.assertEqual(unquote(request.url.path.removeprefix("/")), "MemoChat AI / search?")

    async def test_jina_auth_failure_does_not_leak_provider_payload(self):
        def handler(request: httpx.Request) -> httpx.Response:
            return httpx.Response(
                401,
                text='{"message":"internal provider details"}',
                request=request,
            )

        result = await WebSearchTool(
            api_key="bad-key",
            transport=httpx.MockTransport(handler),
        )._search("MemoChat")

        self.assertIn("JINA_API_KEY", result)
        self.assertNotIn("internal provider details", result)


if __name__ == "__main__":
    unittest.main()
