# AI Agent Trace Panel Plan

Assessed: yes

Summary: make the existing AI Agent layer events visible in the desktop Agent UI through the existing Gate route.

Approach:
- Keep QML talking to the configured Gate `/ai/chat` path.
- Add `events` to Python `ChatRsp`.
- Extend gRPC `AIChatRsp` with trace metadata and serialized event JSON.
- Map trace fields through AIServer and Gate.
- Expose trace properties on `AgentController`.
- Add a focused `AgentTracePane.qml` and open it from `AgentPane.qml`.

Files to modify/create:
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/api/chat_router.py`
- `apps/server/core/common/proto/ai_message.proto`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/GateServer/AIServiceClient.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentTracePane.qml`
- `apps/client/desktop/MemoChat-qml/qml.qrc`

Checklist:
- [x] Capture setup/context.
- [x] Assess existing endpoint and UI paths.
- [x] Add Python `/chat` events.
- [x] Extend C++ chat response contract.
- [x] Add controller trace state.
- [x] Add QML trace panel.
- [x] Run verification and review diff.
