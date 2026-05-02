# Verification Result

Finished: 2026-04-30 10:31:54 -07:00

Commands:

- `python -m compileall apps\server\core\AIOrchestrator\harness\llm\service.py apps\server\core\AIOrchestrator\harness\orchestration\agent_service.py apps\server\core\AIOrchestrator\tools\duckduckgo_tool.py`
  - Passed.
- `$env:PYTHONPATH='.'; .\.venv\Scripts\python.exe tests\test_harness_structure.py`
  - Passed, 29 tests.
- `$env:PYTHONPATH='.'; .\.venv\Scripts\python.exe tests\test_ollama_recovery.py`
  - Passed, 3 tests.
- `cmake --build --preset msvc2022-client-verify`
  - Passed, no work to do.
- `Invoke-WebRequest http://127.0.0.1:8096/health`
  - Returned `{"status":"ok","service":"ai-orchestrator"}`.
- `Invoke-WebRequest http://127.0.0.1:8096/models`
  - Returned only local `ollama/qwen3:4b` plus persisted `api-deepseek` models.
- `POST http://127.0.0.1:8096/chat`
  - Completed with model `qwen3:4b`; trace reached `output:output_ok`.
- `POST http://127.0.0.1:8096/chat/stream`
  - Completed with final SSE event; trace reached `output:output_ok`.
- `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"`
  - AI Orchestrator, Ollama, Qdrant, Neo4j, and related infra are running on stable published ports.
- `docker compose -f apps\server\core\AIOrchestrator\docker-compose.yml config --services`
  - Compose services are `memochat-neo4j`, `memochat-ollama`, `memochat-qdrant`, and `memochat-ai-orchestrator`.

Notes:

- The API provider persistence volume is mounted at `D:/docker-data/memochat/ai-orchestrator/runtime:/app/.data`.
- The mounted provider data was not printed because it contains API credentials.
- Local `qwen3:4b` is slow on this machine but responds for short prompts within the configured runtime probes.
