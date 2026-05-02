# Stage 5 Client Memory Completion Plan

Task summary:
- Expose visible AI memory through Gate/AIServer and add a desktop QML memory management pane.

Approach:
- Mirror the existing knowledge-base contract and request flow.
- Keep delete as Gate `POST /ai/memory/delete`, while AIServer internally calls AIOrchestrator `DELETE /agent/memory/{memory_id}`.

Files to modify:
- `apps/server/core/common/proto/ai_message.proto`
- `apps/server/core/AIServer/AIServiceClient.*`
- `apps/server/core/AIServer/AIServiceCore.*`
- `apps/server/core/AIServer/AIServiceImpl.*`
- `apps/server/core/AIServer/AIServiceJsonMapper.*`
- `apps/server/core/GateServer/AIServiceClient.*`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/client/desktop/MemoChatShared/global.h`
- `apps/client/desktop/MemoChat-qml/AgentController.*`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentMemoryPane.qml`
- `apps/client/desktop/MemoChat-qml/qml.qrc`

Status checklist:
- [x] Context gathered and bridge gap confirmed.
- [x] Plan assessed against existing KB path.
- [x] Add AIServer memory gRPC contract and AIOrchestrator bridge.
- [x] Add Gate `/ai/memory` and `/ai/memory/delete` routes.
- [x] Add AgentController memory state and request handling.
- [x] Add QML memory pane and header entry.
- [x] Run server/client verification builds.
- [x] Review diff and record verification.

Assessed: yes
