# AI Agent Streaming Trace Plan

Assessed: yes

Summary: make streaming chat final chunks include the same trace data as non-streaming chat.

Approach:
- Add trace metadata fields to `AIChatStreamChunk`.
- Add final `feedback_summary`, `observations`, and `events` to Python stream payloads.
- Extend AIServer Python-SSE parsing and gRPC chunk writing.
- Extend Gate gRPC stream reading and HTTP SSE writing.
- Update `AgentController::parseSSEChunk` to refresh trace state from final chunks.

Checklist:
- [x] Capture context and Docker state.
- [x] Add Python final stream trace payload.
- [x] Extend stream proto and C++ callback chain.
- [x] Update client SSE parser.
- [x] Verify Python, server, client, Docker runtime, and diff check.
