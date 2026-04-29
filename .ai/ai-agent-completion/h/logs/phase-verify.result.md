# Verification Result

Commands run:
- `cmake --build --preset msvc2022-server-verify`
- Restarted only runtime `AIServer` on port `8095` after copying `build-verify-server/bin/Release/AIServer.exe`.
- `Invoke-WebRequest http://127.0.0.1:8080/ai/model/list -UseBasicParsing`
- `Invoke-WebRequest http://127.0.0.1:11434/api/tags -UseBasicParsing`
- `git diff --check` on touched files.

Outcomes:
- Server build passed and relinked `AIServer.exe`.
- AIServer restarted and listened on `8095`.
- Gate `/ai/model/list` now returns one enabled Ollama model:
  - `model_type=ollama`
  - `model_name=qwen3:4b`
  - `display_name=Qwen 3 4B`
  - `default_model.model_name=qwen3:4b`
- Ollama `/api/tags` confirms local model `qwen3:4b`.
- `git diff --check` reported no whitespace errors; only existing CRLF normalization warnings.

Runtime ports:
- Gate HTTP/1 `8080` remained listening.
- AIServer `8095` remained listening.
- AIOrchestrator `8096` remained listening in Docker.

Continuation check after interruption:
- Restarted `AIServer` and `GateServer1` when ports `8095`/`8080` were found down.
- Confirmed `AIServer` listening on `8095` and `GateServer1` listening on `8080`.
- Re-ran Gate `/ai/model/list`; it returned only `qwen3:4b` as both the single enabled model and the default model.
