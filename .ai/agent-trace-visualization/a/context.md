# Context

Task: implement first-round Agent trace visualization/tracing improvements: timeline/durations, model input/output previews with masking and expansion, and tool request/response details.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`: creates `TraceEvent`s, has `_model_metadata`, `_tool_execution_metadata`, `_record_tool_observation_events`, and already records `started_at`/`finished_at`.
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`: invokes tools and builds `ToolObservation` metadata. It already masks tool arguments but does not capture duration/status/request/response as a structured call.
- `apps/server/core/AIOrchestrator/harness/llm/service.py`: OpenAI-compatible and Ollama-compatible harness clients; stream chunks can include reasoning content.
- `apps/server/core/AIOrchestrator/llm/base.py`: `LLMResponse` has metadata, `LLMStreamChunk` currently lacks metadata.
- `apps/server/core/AIOrchestrator/llm/manager.py`: fallback/retry manager. This round will not deeply alter fallback behavior unless needed for preview metadata.
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentTracePane.qml`: trace UI currently lists events and metadata rows; duration text exists but no proportional timeline or expandable JSON/prompt detail.
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`: passes trace `events` and observations directly as `QVariantList`.
- `apps/client/desktop/MemoChat-qml/MarkdownRenderer.cpp`: already renders code fences for AI messages. QML can render payload JSON as monospace text without C++ changes in first round.

Docker/MCP checks:
- `docker ps --format ...` shows infrastructure containers running: ai-orchestrator 8096, postgres 15432, redis 6379, ollama 11434, qdrant 6333/6334, neo4j 7474/7687, observability stack, queues, MinIO, Mongo, Redpanda.
- No port/config changes planned.

Existing worktree:
- Many files already modified/untracked before this task, including all trace-related files. Do not revert; edit in place.

Build/test commands to use:
- Python harness tests: from `apps/server/core/AIOrchestrator`, run targeted `python -m unittest tests.test_harness_structure` if dependencies available.
- Client/server CMake build may be heavier; run narrow syntax/tests first, note if full build is not run.

Risks:
- QML JSON object rendering must tolerate QVariantMap/QJSValue values.
- Persisted trace events only preserve `duration_ms` and `created_at`, so historical traces cannot recover exact start if older rows lack new metadata.
- Full system runtime smoke may require live model latency; keep verification short.
