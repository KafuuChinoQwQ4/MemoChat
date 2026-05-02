# Context

## Task

Enhance the AI Agent trace display so users can inspect the execution lifecycle: invocation, orchestration, plan steps, memory/context retrieval, tools, document/search references, model execution, memory write-back, feedback, and node flow.

## Relevant Files

- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`: run and stream orchestration; emits trace events.
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`: executes plan actions and returns `ToolObservation`.
- `apps/server/core/AIOrchestrator/harness/memory/service.py`: loads short-term, episodic, semantic, and graph memory; builds `ContextPack`; saves memory after responses.
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`: persists trace events and projects them into `RunGraph`.
- `apps/server/core/AIOrchestrator/schemas/api.py` and `api/agent_router.py`: expose trace events through `/agent/run`, stream final payloads, `/agent/runs/{trace_id}`, and `/agent/runs/{trace_id}/graph`.
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`: reads `events`, `observations`, `trace_id`, `skill`, and `feedback_summary` from responses.
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentTracePane.qml`: current trace UI.
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`: hosts the trace overlay.

## Existing Behavior

The backend already emits high-level trace events for guardrails, plan, memory load, tool execution, model completion, memory save, and feedback. Events support `metadata`, and persistence keeps it in `ai_agent_run_step.metadata_json`.

The current QML trace pane groups events by layer and shows only event name, status, duration, and summary. It does not surface structured metadata, tool arguments/results, memory sections, document hits, web search summaries, or node-to-node transitions.

## Dependencies

- Docker-backed services: AI Orchestrator, Postgres, Qdrant, Neo4j, Ollama.
- MCP/Docker check used: `docker ps`.

## Verification

Target checks:

- `python -m compileall apps\server\core\AIOrchestrator`
- focused Python harness tests if environment dependencies allow
- client CMake verify if QML resource/build surface changes require it

## Risks

- Do not expose hidden chain-of-thought or secrets in trace detail.
- Existing working tree is dirty with many user changes; edits must stay scoped and avoid broad formatting.
