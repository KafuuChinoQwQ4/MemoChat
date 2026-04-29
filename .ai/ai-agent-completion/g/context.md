# Gate HTTP/1 AI Stream Context

Task: continue the AI Agent work by changing Gate HTTP/1 `/ai/chat/stream` from buffered SSE into true incremental output. The client should receive each AI agent chunk as the upstream stream is read, not only after route completion.

Relevant files:
- `apps/server/core/GateServer/HttpConnection.h`
- `apps/server/core/GateServer/HttpConnection.cpp`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/server/core/GateServer/AIServiceClient.cpp`
- `apps/server/core/AIServer/AIServiceClient.cpp`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/AIOrchestrator/api/chat_router.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/common/proto/ai_message.proto`

Current behavior:
- AIOrchestrator `/chat/stream` returns FastAPI `StreamingResponse` with SSE frames and `X-Accel-Buffering: no`.
- AIServer `AIServiceClient::PostJsonSSE` sends the request to AIOrchestrator but uses `http::read` into `response<string_body>`, so it buffers the whole SSE response before parsing and invoking callbacks.
- AIServer `AIServiceCore::HandleChatStream` writes gRPC chunks as its callback fires.
- Gate `AIServiceClient::ChatStream` reads the server-side gRPC stream chunk by chunk.
- Gate `AIRouteModules.cpp` currently appends each SSE event to `HttpConnection::_response.body()`.
- `HttpConnection::HandleReq` writes the full response after the POST route returns, so Gate HTTP/1 clients do not see chunks incrementally.

Data/service dependencies:
- Docker dependencies are already running on stable ports.
- AIOrchestrator: `memochat-ai-orchestrator` on `8096`.
- AIServer expected on `8095` for Gate gRPC.
- Gate HTTP/1 expected on `8080` in runtime smoke.

Docker checks used:
- `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"` showed AIOrchestrator, Ollama, Qdrant, Neo4j, Postgres, Redis, and observability containers up with stable published ports.

Build/test commands:
- Narrow build: `cmake --preset msvc2022-server-verify`
- Narrow build: `cmake --build --preset msvc2022-server-verify`
- Optional runtime probe after deploy/start if needed: POST `/ai/chat/stream` and observe early SSE chunks.

Open risks:
- Gate route handlers run on `GateWorkerPool` while socket writes must run on the main io_context/socket executor, so streaming writes need explicit queueing.
- HTTP/1 streaming should close the response cleanly with a final chunk and not let `HandleReq` also call the buffered `WriteResponse`.
- AIServer SSE parsing must preserve partial lines across network reads and still populate the final accumulated result for message persistence.
