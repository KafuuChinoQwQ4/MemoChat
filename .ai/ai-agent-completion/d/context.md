# AI Agent Trace Panel Context

Task: expose the AI Agent orchestration, memory, execution, and feedback layer trace in the MemoChat desktop Agent UI without coupling the QML directly to the Python orchestrator.

Relevant files:
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/api/chat_router.py`
- `apps/server/core/common/proto/ai_message.proto`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/GateServer/AIServiceClient.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml.qrc`

Current state:
- Python `/agent/run` already returns `events`.
- Python `/chat` returns trace metadata but not `events`.
- Client uses Gate `/ai/chat`; it should not hardcode the Python orchestrator URL.
- Gate uses gRPC `AIChatRsp`, which currently only carries content, session, tokens, and model.

Dependencies:
- Docker infrastructure remains unchanged. `docker ps` showed AIOrchestrator on `8096` and the existing Postgres/Redis/Mongo/Qdrant/Neo4j/MinIO/observability services on their configured ports.
- No database schema changes are required.

Build/test commands planned:
- Python compile check in `apps/server/core/AIOrchestrator`.
- Server verify CMake configure/build because proto, AIServer, and GateServer change.
- Client verify CMake configure/build because C++ controller and QML resources change.

Open risks:
- The non-streaming chat path will show full trace events first. The streaming path may only have partial trace metadata unless the stream proto is extended later.
- Existing dirty changes belong to prior AI Agent tasks and must not be reverted.
