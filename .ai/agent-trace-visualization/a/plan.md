# Plan

Summary: Add first-round trace transparency for AI Agent runs: proportional timeline, masked model prompt/output details, and structured tool call request/response details.

Approach:
1. Extend backend metadata without changing API schemas: enrich existing `TraceEvent.metadata` dictionaries.
2. Keep sensitive values masked by default. Add explicit metadata flags so UI can label masked payloads.
3. Upgrade `AgentTracePane.qml` only; controller already passes event maps through.
4. Verify via focused Python harness tests and QML/CMake smoke where practical.

Files modified:
- `apps/server/core/AIOrchestrator/llm/base.py`
- `apps/server/core/AIOrchestrator/harness/llm/service.py`
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentTracePane.qml`

Phases:
- [x] Backend metadata enrichment: safe recursive masking, prompt/output preview/full masked fields, stream chunk metadata, tool call detail with duration/status/error/request/response preview.
- [x] UI timeline and details: proportional bars, timestamp/duration text, expand buttons, masked/full preview toggles, JSON payload rendering for tools/model.
- [x] Verification: run targeted Python harness test in Docker; run client configure/build.
- [x] Review diff and record findings.

Docker/MCP checks required:
- Confirm containers via `docker ps`; done, no port edits.

Assessed: yes. Symbols/files exist. Existing controller API carries metadata through without C++ model changes. No database migration required because metadata_json is already JSONB.
