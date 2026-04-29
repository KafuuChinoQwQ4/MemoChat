# Verification

Build:
- `cmake --build --preset msvc2022-full --target MemoChatQml`
- Result: PASS. Built `bin\Release\MemoChatQml.exe`.

Runtime deployment:
- Copied `build\bin\Release\MemoChatQml.exe` to `infra\Memo_ops\runtime\services\MemoChatQml\MemoChatQml.exe` because no `MemoChatQml.exe` process was running.
- Ran `tools\scripts\status\start-all-services.bat`.
- Initially started `AIServer.exe` manually to isolate the failure, then updated the start script to launch AIServer before Gate.
- Re-ran `tools\scripts\status\start-all-services.bat`; it started AIServer and Gate successfully without manual steps.
- Ran `tools\scripts\status\stop-all-services.bat` after verification; Docker containers were left running.

Docker/runtime probes:
- `docker ps` showed `memochat-ai-orchestrator`, `memochat-ollama`, Qdrant, Neo4j, Redis, Postgres, RabbitMQ, and observability containers running.
- `http://127.0.0.1:8096/health` returned `{"status":"ok","service":"ai-orchestrator"}`.
- `http://127.0.0.1:11434/api/tags` returned installed `qwen3:4b`.
- `http://127.0.0.1:8080/ai/model/list` returned code 0 with `Qwen 3 4B` / `qwen3:4b`.
- `http://127.0.0.1:8080/ai/kb/list?uid=1000` returned code 0 with an empty list.
- After the start/stop script fix, one-command startup still returned code 0 with `Qwen 3 4B` / `qwen3:4b`.

Not run:
- Manual screenshot verification of the QML UI.
