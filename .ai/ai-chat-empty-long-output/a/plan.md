# Plan

## Goal

When a long algorithm prompt produces no visible streamed content, the backend should either send the recovered retry answer to the UI or send a clear final model-empty message. It must not silently recover internally while the SSE client sees an empty answer.

## Steps

1. Inspect AI chat streaming path and Ollama adapter.
2. Fix empty-stream retry so recovered non-streaming content is emitted as an SSE chunk.
3. Add a retry hint that forces direct final answer and disables thinking for the retry.
4. Replace fragile per-chunk `<think>` stripping with a stateful parser that preserves answer text after `</think>`.
5. Add regression tests for recovered retry content and split/same-chunk think tags.
6. Verify with syntax check, container unit test, and Docker-backed runtime smoke.

## Success Criteria

- Stream retry content is visible to the client.
- Empty stream fallback uses `think=False` and an explicit no-thinking hint.
- Answer text after a closing think tag is preserved.
- `tests.test_harness_structure` passes inside `memochat-ai-orchestrator`.
- Runtime `/chat/stream` returns visible SSE chunks for a Chinese algorithm prompt.
