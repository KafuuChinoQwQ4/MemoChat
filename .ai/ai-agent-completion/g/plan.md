# Gate HTTP/1 AI Stream Plan

Assessed: yes

Summary:
- Make AI agent streaming end-to-end incremental: AIOrchestrator SSE read incrementally in AIServer, gRPC chunks forwarded immediately, and Gate HTTP/1 writes chunked SSE frames immediately.

Approach:
- Add an HTTP/1 chunked streaming response mode to `HttpConnection`.
- Change `/ai/chat/stream` to call the streaming mode for valid requests, write each SSE event through the new chunk writer, and finish the stream after gRPC completion.
- Change AIServer `PostJsonSSE` from whole-body `http::read` to `read_header` plus repeated `read_some`, parsing SSE lines as bytes arrive.

Files to modify:
- `apps/server/core/GateServer/HttpConnection.h`
- `apps/server/core/GateServer/HttpConnection.cpp`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/server/core/AIServer/AIServiceClient.cpp`

Implementation phases:
- [x] Add `HttpConnection` stream state, public `StartSseStream`, `WriteStreamChunk`, `FinishStream`, and `HasStreamingResponse`.
- [x] Update POST completion path to skip buffered `WriteResponse` when a route has taken over streaming.
- [x] Update `/ai/chat/stream` route to use the stream writer and finish even on gRPC errors.
- [x] Update AIServer SSE client to incrementally read and parse upstream frames.
- [x] Build server verification and review diff.

Docker/MCP checks required:
- Keep Docker published ports unchanged.
- No database or schema changes expected.

Verification:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
