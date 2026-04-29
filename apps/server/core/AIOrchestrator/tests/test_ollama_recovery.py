from __future__ import annotations

import json
import unittest
from unittest.mock import AsyncMock

import httpx

from llm.base import LLMMessage
from llm.ollama_llm import OllamaLLM


class _FakeClient:
    def __init__(self, post_response: httpx.Response | None = None, stream_response=None):
        self._post_response = post_response
        self._stream_response = stream_response
        self.is_closed = False

    async def post(self, url: str, json: dict):
        return self._post_response

    def stream(self, method: str, url: str, json: dict):
        return self._stream_response

    async def aclose(self):
        self.is_closed = True


class _FakeStreamResponse:
    def __init__(self, response: httpx.Response, lines: list[str] | None = None):
        self._response = response
        self._lines = lines or []

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc, tb):
        return False

    def raise_for_status(self):
        self._response.raise_for_status()

    async def aiter_lines(self):
        for line in self._lines:
            yield line


class OllamaRecoveryTests(unittest.IsolatedAsyncioTestCase):
    async def test_chat_retries_once_after_transient_404(self):
        first = httpx.Response(
            404,
            request=httpx.Request("POST", "http://ollama/api/chat"),
            text="404 page not found",
        )
        second = httpx.Response(
            200,
            request=httpx.Request("POST", "http://ollama/api/chat"),
            json={
                "message": {"content": "hello"},
                "prompt_eval_count": 3,
                "eval_count": 5,
                "done_reason": "stop",
            },
        )
        llm = OllamaLLM(base_url="http://ollama", model_name="qwen3:4b")
        llm._get_client = AsyncMock(side_effect=[_FakeClient(post_response=first), _FakeClient(post_response=second)])
        llm._reset_client = AsyncMock()
        llm._wait_until_ready = AsyncMock()

        result = await llm.chat([LLMMessage(role="user", content="hi")])

        self.assertEqual(result.content, "hello")
        self.assertEqual(result.usage.total_tokens, 8)
        llm._reset_client.assert_awaited_once()
        llm._wait_until_ready.assert_awaited_once()

    async def test_chat_does_not_retry_model_not_found_404(self):
        missing_model = httpx.Response(
            404,
            request=httpx.Request("POST", "http://ollama/api/chat"),
            text='{"error":"model \\"missing\\" not found"}',
        )
        llm = OllamaLLM(base_url="http://ollama", model_name="missing")
        llm._get_client = AsyncMock(return_value=_FakeClient(post_response=missing_model))
        llm._reset_client = AsyncMock()
        llm._wait_until_ready = AsyncMock()

        with self.assertRaises(RuntimeError) as ctx:
            await llm.chat([LLMMessage(role="user", content="hi")])

        self.assertIn("Ollama HTTP error: 404", str(ctx.exception))
        llm._reset_client.assert_not_awaited()
        llm._wait_until_ready.assert_not_awaited()

    async def test_chat_stream_retries_once_before_emitting_chunks(self):
        first_response = httpx.Response(
            404,
            request=httpx.Request("POST", "http://ollama/api/chat"),
            text="404 page not found",
        )
        second_response = httpx.Response(
            200,
            request=httpx.Request("POST", "http://ollama/api/chat"),
        )
        stream_lines = [
            json.dumps({"message": {"content": "hello"}, "done": False}),
            json.dumps({"message": {"content": " world"}, "done": True, "prompt_eval_count": 2, "eval_count": 4}),
        ]
        llm = OllamaLLM(base_url="http://ollama", model_name="qwen3:4b")
        llm._get_client = AsyncMock(
            side_effect=[
                _FakeClient(stream_response=_FakeStreamResponse(first_response)),
                _FakeClient(stream_response=_FakeStreamResponse(second_response, lines=stream_lines)),
            ]
        )
        llm._reset_client = AsyncMock()
        llm._wait_until_ready = AsyncMock()

        chunks = []
        async for chunk in llm.chat_stream([LLMMessage(role="user", content="hi")]):
            chunks.append(chunk)

        self.assertEqual([chunk.content for chunk in chunks], ["hello", "world"])
        self.assertFalse(chunks[0].is_final)
        self.assertTrue(chunks[1].is_final)
        self.assertEqual(chunks[1].usage.total_tokens, 6)
        llm._reset_client.assert_awaited_once()
        llm._wait_until_ready.assert_awaited_once()


if __name__ == "__main__":
    unittest.main()
