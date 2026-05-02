# Context

Task: diagnose and fix `All LLM backends failed for streaming. Tried: ['ollama']` during an AI chat test.

Relevant files:
- `apps/server/core/AIOrchestrator/llm/ollama_llm.py`: hard-coded `httpx.Timeout(120.0)` and stream/chat retry logging.
- `apps/server/core/AIOrchestrator/config.py`: `OllamaLLMConfig` lacks timeout setting.
- `apps/server/core/AIOrchestrator/config.yaml`: `llm.ollama` runtime config mounted read-only into AIOrchestrator container.
- `apps/server/core/AIOrchestrator/llm/manager.py`: wraps backend stream failures into `AllBackendsFailedError`; currently logs only `str(e)`.
- `apps/server/core/AIOrchestrator/tests/test_ollama_recovery.py`: targeted Ollama recovery tests.

Docker checks:
- `docker ps` shows AIOrchestrator and Ollama healthy; no port drift.
- `Invoke-WebRequest http://127.0.0.1:8096/health` returns ok.
- `Invoke-WebRequest http://127.0.0.1:11434/api/tags` returns qwen3:4b.
- `docker exec memochat-ollama ollama ps` shows qwen3:4b loaded on 100% CPU.

Runtime probe:
- From `memochat-ai-orchestrator`, direct `httpx.stream` to `http://memochat-ollama:11434/api/chat` with `hi` and `num_predict=8` returned status 200 only after ~65.8s.
- Earlier long prompt requests hit 120s timeout; Ollama logged 500 because client closed connection.

Risk:
- Raising timeout improves slow CPU behavior but can make genuinely stuck requests wait longer. Keep total agent timeout constraints in mind for future cancellation UX.
