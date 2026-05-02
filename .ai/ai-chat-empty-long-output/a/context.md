# Context

## User Report

AI 聊天界面测试算法题时，长时间生成后显示：

> 模型本次没有返回可显示内容。请重试，或关闭思考模式/切换模型后再试。

简单问题可以正常输出，因此基础 HTTP/SSE/模型链路可用，问题集中在复杂题、长输出、thinking 内容过滤和空内容恢复路径。

## Relevant Runtime

- AIOrchestrator container: `memochat-ai-orchestrator`, published on `127.0.0.1:8096`, healthy.
- Ollama container: `memochat-ollama`, published on `127.0.0.1:11434`, healthy.
- Main endpoint: `POST http://127.0.0.1:8096/chat/stream`.
- Request model fields include `model_type`, `model_name`, `deployment_preference`, and `metadata.think`.

## Root Cause

Two backend streaming issues could create an empty visible answer for long or algorithm prompts:

1. `AgentHarnessService.stream_turn()` had a fallback path for empty streamed output. It retried with non-streaming `complete()` and assigned the recovered answer to `accumulated`, but did not yield that recovered content to the SSE client before the final event. The trace could internally finish with content while the UI still received no visible chunk.

2. `OllamaLLM.chat_stream()` stripped `<think>...</think>` on a per-chunk basis. If a chunk contained both the closing tag and answer text, for example `</think>最终答案`, the old state handling could discard the visible answer portion.

## Fix Scope

- Backend streaming recovery only.
- Ollama think-tag streaming parser.
- Focused unit tests in `tests.test_harness_structure`.
- Runtime smoke through Docker-backed AIOrchestrator.

No QML code change was required for the observed blank-output symptom because the server was not reliably sending recovered visible content.
