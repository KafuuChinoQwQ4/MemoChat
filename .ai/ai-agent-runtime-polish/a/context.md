# Context

## Task

Fix a runtime test where the UI showed `请求被安全护栏拦截: model output is empty`. Also add assistant message code-block adaptation, persist API provider model lists after registration, and remove unavailable local models such as Qwen variants not installed in Ollama.

## Findings

- `apps/server/core/AIOrchestrator/tools/duckduckgo_tool.py` uses `async for ddgs.atext(...)`, but the installed `duckduckgo_search` returns a coroutine for `atext()`. Container logs show `coroutine 'AsyncDDGS.atext' was never awaited`.
- `apps/server/core/AIOrchestrator/harness/llm/service.py` strips `<think>` blocks in streaming mode. If the model stream yields only thinking or parsing strips all visible content, `AgentHarnessService` later passes empty content to output guardrails, which produces the misleading safety-block message.
- Ollama currently reports only `qwen3:4b` via `http://127.0.0.1:11434/api/tags`; `config.yaml` still lists `qwen2.5:3b` and `qwen2.5-coder:3b`.
- Runtime API providers are saved under `/app/.data/api_providers.json` inside the container, but `.data` is not mounted in `docker-compose.yml`, so provider model lists can disappear after container recreation/rebuild.
- Client model list is held in memory only by `AgentController`; adding a `QSettings` cache provides UI-side resilience.
- `MarkdownRenderer` already supports fenced code blocks, but the output is a simple `<pre>` with no language label or wrapping adaptation.

## Relevant Files

- `apps/server/core/AIOrchestrator/harness/llm/service.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/tools/duckduckgo_tool.py`
- `apps/server/core/AIOrchestrator/config.yaml`
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `apps/client/desktop/MemoChat-qml/MarkdownRenderer.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`

## Docker Checks

- AI Orchestrator logs inspected via Docker.
- Ollama `/api/tags` returned one installed model: `qwen3:4b`.

## Risks

- Do not expose API keys in logs or final response.
- Docker compose volume change requires redeploy/recreate to take effect.
