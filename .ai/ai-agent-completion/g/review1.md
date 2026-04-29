# Review 1

Findings:
- No blocking correctness issues found in the touched streaming path.

Checks:
- Gate `/ai/chat/stream` now takes over the HTTP response with `StartSseStream`, writes each serialized SSE event through `WriteStreamChunk`, and calls `FinishStream` after the AIServer gRPC stream completes.
- `HttpConnection` skips the normal buffered `WriteResponse` when a route has taken over streaming.
- Stream writes are marshalled back to the socket executor and serialized through a queue, so gRPC worker callbacks do not write to the socket directly.
- The HTTP/1 response uses chunked transfer, `Cache-Control: no-cache`, `X-Accel-Buffering: no`, `Connection: close`, and `TCP_NODELAY`.
- AIServer now reads the AIOrchestrator SSE response with `read_header` plus repeated `read_some`, preserving partial lines in `pending` and invoking the chunk callback as each `data:` line arrives.
- The final accumulated result is still populated for AIServer message persistence.

Residual risks:
- Invalid JSON and empty-content errors still use the existing buffered SSE-shaped one-frame response. The requested optimization targeted valid AI streaming output.
- The runtime model list still advertises `qwen2.5` models while the current Ollama container only has `qwen3:4b`; this is a pre-existing config/runtime mismatch and was worked around for the stream probe.
