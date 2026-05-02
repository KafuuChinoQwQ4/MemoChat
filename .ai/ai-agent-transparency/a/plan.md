# Plan

## Summary

Make AI Agent traces more transparent by enriching backend trace events and improving the QML trace pane presentation.

## Approach

Use existing `TraceEvent.metadata` and persisted run graph projection. Add structured, safe event metadata and per-observation tool events. Improve QML rendering so metadata is readable without requiring JSON spelunking.

## Files To Modify

- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/harness/memory/service.py`
- `apps/server/core/AIOrchestrator/harness/ports.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentTracePane.qml`

## Phases

- [x] Enrich trace event metadata and add invocation/node summaries.
- [x] Add structured tool, knowledge, web, graph, and memory write-back details.
- [x] Upgrade QML trace pane to show flow, detail, and metadata summaries.
- [x] Run compile/test verification.
- [x] Review diff and record results.

## Docker/MCP Checks

- `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"` completed; required containers are running and fixed ports are unchanged.

## Build/Test Commands

- `python -m compileall apps\server\core\AIOrchestrator`
- `python apps\server\core\AIOrchestrator\tests\test_harness_structure.py`
- Consider `cmake --preset msvc2022-client-verify` if QML syntax risk remains.

Assessed: yes
