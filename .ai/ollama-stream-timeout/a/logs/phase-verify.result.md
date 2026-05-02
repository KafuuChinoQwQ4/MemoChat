# Verification

Commands and results:

```powershell
python -m py_compile config.py llm/ollama_llm.py llm/manager.py
```
Result: passed after fixing an indentation typo in `llm/manager.py`.

```powershell
python -m unittest tests.test_ollama_recovery
```
Host result: blocked by missing `structlog`, same local dependency issue seen previously.

```powershell
docker compose -f apps/server/core/AIOrchestrator/docker-compose.yml up -d --build memochat-ai-orchestrator
```
Result: AIOrchestrator image rebuilt and container recreated; qdrant/neo4j/ollama stayed on existing stable ports.

```powershell
docker exec memochat-ai-orchestrator sh -lc "cd /app && python -m unittest tests.test_ollama_recovery"
```
Result: passed, 3 tests.

```powershell
docker exec memochat-ai-orchestrator sh -lc "cd /app && python - <<'PY' ... settings.llm.ollama.timeout_sec ..."
```
Result: container reads `timeout 300`; loaded backend has `backend_timeout 300`.

```powershell
docker exec memochat-ai-orchestrator sh -lc "python - <<'PY' ... POST http://127.0.0.1:8096/chat/stream ..."
```
Result: status 200; stream chunks received from the real AIOrchestrator route.

Diagnostics before fix:
- `qwen3:4b` exists in Ollama.
- Direct minimal Ollama stream from AIOrchestrator took ~65.8s before first line, so long prompts can exceed the old 120s timeout.
- Ollama logged `/api/chat` 500 after 2m0s because the client timed out and closed the connection.

Docker port/config drift:
- No published port changes. `memochat-ai-orchestrator` remains on 8096 and `memochat-ollama` on 11434.
