# Context

Task: complete the remaining AI Agent engineering follow-ups after backend hardening and UI/runtime-config stabilization:

- add a repeatable Gate-level AI smoke script
- add minimal AIOrchestrator metrics and Prometheus scrape wiring
- migrate Ollama model storage to `D:` and reduce compose/runtime drift warnings where feasible

Relevant files:

- `tools/scripts/*`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/server/core/AIOrchestrator/main.py`
- `apps/server/core/AIOrchestrator/api/chat_router.py`
- `apps/server/core/AIOrchestrator/api/kb_router.py`
- `apps/server/core/AIOrchestrator/api/model_router.py`
- `apps/server/core/AIOrchestrator/llm/ollama_llm.py`
- `infra/deploy/local/observability/prometheus/prometheus.yml`
- `apps/server/core/AIOrchestrator/docker-compose.yml`

Current findings:

- Gate AI HTTP routes are simple and do not enforce token checks on `/ai/*`; they require `uid` for chat/session/KB routes and no auth for `/ai/model/list`.
- Local GateServer is currently reachable on `http://127.0.0.1:8080`, and `/ai/model/list` already returns qwen3-only data.
- AIOrchestrator currently has `/health` and `/ready`, but no Prometheus-style `/metrics`.
- Prometheus local config currently does not scrape AIOrchestrator.
- Ollama model store is still mounted from repo-local `apps/server/core/AIOrchestrator/.data/ollama`.
- Current Ollama store size is about `2.5 GB`, with one model:
  - `qwen3:4b`
- Target `D:/docker-data/memochat/ollama` does not exist yet.
- During runtime smoke, unbounded Gate chat requests against local CPU Ollama can exceed 120 seconds. The fix is to pass chat `metadata` through Gate/AIServer and let smoke cap `max_tokens`.

Checks used:

- `Invoke-WebRequest http://127.0.0.1:8080/ai/model/list`
- `docker exec memochat-ollama ollama list`
- file-size probe on `apps/server/core/AIOrchestrator/.data/ollama`
- `Get-NetTCPConnection -State Listen -LocalPort 8080,8096`
- `powershell -ExecutionPolicy Bypass -File tools/scripts/ai_agent_smoke.ps1`
- `Invoke-WebRequest http://127.0.0.1:8096/metrics`
- Prometheus query `up{job="ai_orchestrator"}`

Verification plan:

- run the new smoke script against local GateServer + AIOrchestrator
- run `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest ...`
- verify `/metrics` output and Prometheus reload
- recreate `memochat-ollama` / `memochat-ai-orchestrator` after mount migration

Open risks:

- Marking Neo4j / Qdrant named volumes as external could suppress compose warnings but would make fresh environments depend on pre-created volumes; avoid unless the tradeoff is clearly worth it.
- Ollama store migration is a real data move; verify destination contents before switching the bind mount.
- Manual Agent/KnowledgeBase UI still needs human visual execution against the running client; checklist is tracked in `manual-ui-checklist.md`.
