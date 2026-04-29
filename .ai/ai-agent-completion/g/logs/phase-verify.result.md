# Verification Result

Commands run:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- Restarted runtime `AIServer` and `GateServer1` only, copying `build-verify-server/bin/Release/AIServer.exe` and `GateServer.exe` into their existing runtime service directories.
- `Invoke-WebRequest http://127.0.0.1:8080/ai/model/list -UseBasicParsing`
- Docker dependency probes:
  - `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"`
  - `docker exec memochat-redis redis-cli -a 123456 ping`
  - `docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"`
- HTTP streaming probe using .NET `HttpClient` with `ResponseHeadersRead` against `http://127.0.0.1:8080/ai/chat/stream`.

Outcomes:
- Configure passed.
- Server build passed: `137/137` targets completed, including `GateServer.exe` and `AIServer.exe`.
- Runtime ports after restart:
  - Gate HTTP/1 `8080` listening.
  - AIServer `8095` listening.
  - AIOrchestrator `8096` listening in Docker.
- `/ai/model/list` returned `code=0`.
- Redis returned `PONG`; Postgres returned `select 1`.
- Docker published ports remained stable.

Streaming probe:
- First attempt used `qwen2.5:7b`; local Ollama only had `qwen3:4b`, so AIOrchestrator returned an Ollama `404` and Gate streamed a final error frame. This still showed chunked headers, but not normal multi-frame generation.
- Second attempt used `model_name=qwen3:4b`.
- Gate returned `status=200`, `Transfer-Encoding: chunked=True`, and response headers arrived at `416ms`.
- The first normal content frame arrived at `32271ms`.
- The stream produced `301` SSE frames, with many content frames arriving roughly every `100-200ms`, and final frame at `82268ms`.
- Result: PASS for true incremental HTTP/1 SSE output through Gate.

Notes:
- Runtime `GateServer1` and `AIServer` are left running on their stable ports with the newly built binaries.
- Gate HTTP/3 startup still logs the pre-existing local certificate credential warning; this did not affect HTTP/1 stream verification.
