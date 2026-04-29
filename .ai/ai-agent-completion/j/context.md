# Context

Task: continue AI Agent completion with engineering hardening rather than new mainline features:

- Add minimal regression coverage around AIServer model list and knowledge-base list/delete JSON mapping so future field-shape regressions are caught.
- Investigate the intermittent AIOrchestrator `ollama /api/chat` `404` failure and harden restart/self-recovery behavior after backend or Ollama restarts.

Relevant files:

- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/AIServer/AIServiceClient.cpp`
- `apps/server/core/AIOrchestrator/llm/ollama_llm.py`
- `apps/server/core/AIOrchestrator/llm/manager.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `apps/server/core/AIOrchestrator/config.yaml`
- `apps/server/core/AIOrchestrator/docker-compose.yml`

Current findings:

- AIServer forwarding for `/models`, `/kb/list`, and `DELETE /kb/{kb_id}` already exists in the dirty tree, but the JSON mapping is still embedded in `AIServiceCore.cpp` and is not covered by tests.
- Prior verification already found one regression where array parsing returned empty results; this is exactly the kind of issue that should be locked down with a pure mapping test.
- AIOrchestrator logs include an intermittent streaming failure at `2026-04-28 16:08:23`:
  - `llm.astream_error backend=ollama error=Client error '404 Not Found' for url 'http://memochat-ollama:11434/api/chat'`
  - immediately followed by `harness.stream.failed error=All LLM backends failed for streaming. Tried: ['ollama']`
- Current Ollama adapters call `/api/chat` directly and surface the exception, but they do not reset the cached `httpx.AsyncClient`, probe readiness, or retry once after a transient startup/restart failure.
- Current AIOrchestrator and Ollama containers are both running with stable ports, but there is no evidence of container-level restart counts in the current instance (`RestartCount=0` for both).

Docker / MCP checks used:

- `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"`
- `docker logs --tail 200 memochat-ai-orchestrator`
- `docker logs --tail 200 memochat-ollama`
- `docker inspect memochat-ai-orchestrator --format "{{.RestartCount}} {{.State.StartedAt}} {{.State.Status}}"`
- `docker inspect memochat-ollama --format "{{.RestartCount}} {{.State.StartedAt}} {{.State.Status}}"`

Build / test plan:

- `cmake --preset msvc2022-tests`
- `cmake --build --preset msvc2022-tests`
- `ctest --preset msvc2022-tests --output-on-failure`
- `python -m unittest apps.server.core.AIOrchestrator.tests.test_harness_structure`
- If needed, a narrower Python test invocation for the new Ollama recovery tests.

Open risks:

- The repository still has a config/runtime consistency risk for AIOrchestrator model inventory after container rebuilds; that is adjacent but not the primary scope of this phase.
- AIServer tests should avoid coupling to live Postgres or HTTP calls; extracting pure mapping helpers is the lowest-risk path.
