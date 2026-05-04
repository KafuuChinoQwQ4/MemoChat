"""
Ollama LLM 适配器 — 支持本地大模型
"""
import asyncio
import httpx
import json
import re
from typing import AsyncIterator

import structlog

from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage
from observability.metrics import ai_metrics

logger = structlog.get_logger()


class OllamaLLM(BaseLLM):
    _instance: "OllamaLLM | None" = None
    _RECOVERABLE_STATUS_CODES = {404, 502, 503, 504}

    def __init__(
        self,
        base_url: str = "http://127.0.0.1:11434",
        model_name: str = "qwen3:4b",
        timeout_sec: int = 300,
    ):
        super().__init__(model_name)
        self.base_url = base_url.rstrip("/")
        self.timeout_sec = timeout_sec
        self._client: httpx.AsyncClient | None = None
        OllamaLLM._instance = self

    @staticmethod
    def _strip_think_blocks(content: str) -> str:
        if not content:
            return ""
        if "</think>" in content:
            content = content.rsplit("</think>", 1)[-1]
        content = re.sub(r"<think>.*?</think>\s*", "", content, flags=re.DOTALL)
        return content.strip()

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=httpx.Timeout(float(self.timeout_sec)))
        return self._client

    async def _reset_client(self) -> None:
        client = self._client
        self._client = None
        if client is not None and not client.is_closed:
            await client.aclose()

    @classmethod
    def _is_recoverable_status(cls, response: httpx.Response) -> bool:
        if response.status_code not in cls._RECOVERABLE_STATUS_CODES:
            return False
        body = response.text.lower()
        if response.status_code == 404 and "model" in body and "not found" in body:
            return False
        return True

    @staticmethod
    def _response_excerpt(response: httpx.Response) -> str:
        text = response.text.strip()
        if len(text) <= 200:
            return text
        return text[:200] + "..."

    async def _wait_until_ready(self, attempts: int = 3) -> None:
        last_error: Exception | None = None
        for probe_attempt in range(1, attempts + 1):
            await asyncio.sleep(0.25 * probe_attempt)
            await self._reset_client()
            client = await self._get_client()
            try:
                resp = await client.get(f"{self.base_url}/api/tags")
                resp.raise_for_status()
                ai_metrics.ollama_ready_probes.inc(status="ok")
                logger.info("ollama.ready_after_retry", probe_attempt=probe_attempt, base_url=self.base_url)
                return
            except Exception as exc:
                last_error = exc
                ai_metrics.ollama_ready_probes.inc(status="error")
                logger.warning(
                    "ollama.ready_probe_failed",
                    probe_attempt=probe_attempt,
                    base_url=self.base_url,
                    error=str(exc),
                )

        if last_error is not None:
            logger.warning("ollama.ready_probe_exhausted", base_url=self.base_url, error=str(last_error))

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        payload = {
            "model": self.model_name,
            "messages": self._messages_to_dict(messages),
            "stream": False,
            "think": bool(kwargs.get("think", False)),
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }

        for attempt in range(2):
            client = await self._get_client()
            try:
                resp = await client.post(f"{self.base_url}/api/chat", json=payload)
                resp.raise_for_status()
                data = resp.json()

                raw_content = data.get("message", {}).get("content", "")
                content = raw_content if kwargs.get("think", False) else self._strip_think_blocks(raw_content)
                usage_data = data.get("eval_count", 0)
                prompt_eval_count = data.get("prompt_eval_count", 0)

                return LLMResponse(
                    content=content,
                    usage=LLMUsage(
                        prompt_tokens=prompt_eval_count,
                        completion_tokens=usage_data,
                        total_tokens=prompt_eval_count + usage_data,
                    ),
                    model=self.model_name,
                    finish_reason=data.get("done_reason", ""),
                )
            except httpx.HTTPStatusError as exc:
                if attempt == 0 and self._is_recoverable_status(exc.response):
                    ai_metrics.ollama_retries.inc(
                        operation="chat",
                        reason=f"http_{exc.response.status_code}",
                    )
                    logger.warning(
                        "ollama.chat_retry",
                        attempt=attempt + 1,
                        status_code=exc.response.status_code,
                        body=self._response_excerpt(exc.response),
                        base_url=self.base_url,
                    )
                    await self._reset_client()
                    await self._wait_until_ready()
                    continue
                raise RuntimeError(f"Ollama HTTP error: {exc.response.status_code} — {exc.response.text}") from exc
            except httpx.RequestError as exc:
                if attempt == 0:
                    ai_metrics.ollama_retries.inc(operation="chat", reason="request_error")
                    logger.warning(
                        "ollama.chat_retry",
                        attempt=attempt + 1,
                        error=str(exc),
                        base_url=self.base_url,
                    )
                    await self._reset_client()
                    await self._wait_until_ready()
                    continue
                raise RuntimeError(f"Ollama request failed: {exc}") from exc
            except Exception as exc:
                raise RuntimeError(f"Ollama request failed: {exc}") from exc

        raise RuntimeError("Ollama request failed after recovery attempt")

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        payload = {
            "model": self.model_name,
            "messages": self._messages_to_dict(messages),
            "stream": True,
            "think": bool(kwargs.get("think", False)),
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }

        for attempt in range(2):
            client = await self._get_client()
            yielded_any = False
            try:
                async with client.stream("POST", f"{self.base_url}/api/chat", json=payload) as resp:
                    resp.raise_for_status()
                    accumulated = ""
                    total_tokens = 0
                    in_think_block = False

                    async for line in resp.aiter_lines():
                        if not line.strip():
                            continue
                        try:
                            data = json.loads(line)
                        except json.JSONDecodeError:
                            continue

                        delta = data.get("message", {}).get("content", "")
                        if not kwargs.get("think", False):
                            if "<think>" in delta:
                                in_think_block = True
                                delta = delta.split("<think>", 1)[0]
                            if in_think_block and "</think>" in delta:
                                delta = delta.split("</think>", 1)[1]
                                in_think_block = False
                            elif in_think_block:
                                delta = ""
                            delta = self._strip_think_blocks(delta)
                        accumulated += delta

                        eval_count = data.get("eval_count", 0)
                        prompt_eval_count = data.get("prompt_eval_count", 0)
                        total_tokens = eval_count + prompt_eval_count
                        done = data.get("done", False)
                        yielded_any = True

                        yield LLMStreamChunk(
                            content=delta,
                            is_final=done,
                            usage=LLMUsage(
                                prompt_tokens=prompt_eval_count,
                                completion_tokens=eval_count,
                                total_tokens=total_tokens,
                            ) if done else None,
                            model=self.model_name,
                        )

                        if done:
                            return
                    return
            except httpx.HTTPStatusError as exc:
                if attempt == 0 and not yielded_any and self._is_recoverable_status(exc.response):
                    ai_metrics.ollama_retries.inc(
                        operation="stream",
                        reason=f"http_{exc.response.status_code}",
                    )
                    logger.warning(
                        "ollama.stream_retry",
                        attempt=attempt + 1,
                        status_code=exc.response.status_code,
                        body=self._response_excerpt(exc.response),
                        base_url=self.base_url,
                    )
                    await self._reset_client()
                    await self._wait_until_ready()
                    continue
                raise RuntimeError(f"Ollama HTTP error: {exc.response.status_code} — {exc.response.text}") from exc
            except httpx.RequestError as exc:
                if attempt == 0 and not yielded_any:
                    ai_metrics.ollama_retries.inc(operation="stream", reason="request_error")
                    logger.warning(
                        "ollama.stream_retry",
                        attempt=attempt + 1,
                        error=str(exc),
                        base_url=self.base_url,
                    )
                    await self._reset_client()
                    await self._wait_until_ready()
                    continue
                raise RuntimeError(f"Ollama request failed: {exc}") from exc
            except Exception as exc:
                raise RuntimeError(f"Ollama request failed: {exc}") from exc

        raise RuntimeError("Ollama request failed after recovery attempt")

    async def list_models(self) -> list[str]:
        client = await self._get_client()
        try:
            resp = await client.get(f"{self.base_url}/api/tags")
            resp.raise_for_status()
            data = resp.json()
            return [m["name"] for m in data.get("models", [])]
        except Exception:
            return [self.model_name]

    def close(self):
        if self._client and not self._client.is_closed:
            asyncio.create_task(self._client.aclose())
