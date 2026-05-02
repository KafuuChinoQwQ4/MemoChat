# Context

Task: add a delete function for API-connected large models shown in the model list.

Relevant flow:
- Desktop QML model settings: `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- Desktop controller: `apps/client/desktop/MemoChat-qml/AgentController.cpp/.h`
- Request IDs: `apps/client/desktop/MemoChatShared/global.h`
- Gate HTTP route: `apps/server/core/GateServer/AIRouteModules.cpp`
- Gate gRPC client: `apps/server/core/GateServer/AIServiceClient.cpp/.h`
- AIServer proto/core/impl/client mapper:
  - `apps/server/core/common/proto/ai_message.proto`
  - `apps/server/core/AIServer/AIServiceImpl.cpp/.h`
  - `apps/server/core/AIServer/AIServiceCore.cpp/.h`
  - `apps/server/core/AIServer/AIServiceClient.cpp/.h`
  - `apps/server/core/AIServer/AIServiceJsonMapper.cpp/.h`
- AIOrchestrator route/service/schema:
  - `apps/server/core/AIOrchestrator/api/model_router.py`
  - `apps/server/core/AIOrchestrator/harness/llm/service.py`
  - `apps/server/core/AIOrchestrator/schemas/api.py`

Existing behavior:
- Register API provider writes runtime provider config in AIOrchestrator.
- `ListModels` returns all enabled endpoints, including runtime providers.
- Runtime provider IDs are normalized to `api-*`.
- Desktop model list has no delete action yet.

Docker/MCP:
- `docker ps` checked. AIOrchestrator and infrastructure containers are up; no port changes needed.

Verification plan:
- Python compile for touched AIOrchestrator files.
- AIOrchestrator unit test if relevant tests remain fast.
- CMake server verify for proto/Gate/AIServer.
- CMake client verify for QML/client changes.
