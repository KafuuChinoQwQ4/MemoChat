# Plan

Task summary: finish the last AI Agent engineering tasks with runtime smoke coverage, minimal observability, and model-storage migration.

Approach:

- Add a PowerShell smoke script that exercises Gate `/ai/model/list`, `/ai/chat`, `/ai/chat/stream`, `/ai/kb/upload`, `/ai/kb/list`, `/ai/kb/search`, and `/ai/kb/delete`.
- Add a lightweight in-memory metrics registry inside AIOrchestrator and expose `/metrics` without pulling in extra dependencies.
- Add Prometheus scrape wiring for AIOrchestrator metrics.
- Migrate Ollama model data from repo-local `.data/ollama` to `D:/docker-data/memochat/ollama`, then update compose to use the new mount.
- Add Gate -> AIServer -> AIOrchestrator chat metadata passthrough so smoke can cap local Ollama output tokens.

Files to modify/create:

- `tools/scripts/ai_agent_smoke.ps1`
- `apps/server/core/AIOrchestrator/observability/metrics.py`
- `apps/server/core/AIOrchestrator/api/chat_router.py`
- `apps/server/core/AIOrchestrator/api/kb_router.py`
- `apps/server/core/AIOrchestrator/api/model_router.py`
- `apps/server/core/AIOrchestrator/llm/ollama_llm.py`
- `apps/server/core/AIOrchestrator/main.py`
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `infra/deploy/local/observability/prometheus/prometheus.yml`
- `apps/server/core/common/proto/ai_message.proto`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/server/core/GateServer/AIServiceClient.{h,cpp}`
- `apps/server/core/AIServer/AIServiceClient.{h,cpp}`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `.ai/ai-agent-completion/l/*`

Implementation phases:

- [x] Write follow-up context and plan.
- [x] Add a Gate-level AI smoke script.
- [x] Add minimal AI metrics and `/metrics` exposure.
- [x] Wire Prometheus scrape config for AIOrchestrator metrics.
- [x] Migrate Ollama model store to `D:` and update compose mount.
- [x] Add chat metadata passthrough for bounded smoke generation.
- [x] Run targeted verification and review.

Assessed: yes
