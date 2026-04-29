Task: add a UI and backend path to connect external model APIs such as GPT by entering an official API base URL and API key, auto-parse available models, and make them selectable in AI Agent.

Relevant files:
- apps/server/core/AIOrchestrator/api/model_router.py
- apps/server/core/AIOrchestrator/harness/llm/service.py
- apps/server/core/AIOrchestrator/schemas/api.py
- apps/server/core/common/proto/ai_message.proto
- apps/server/core/AIServer/AIServiceClient.h/.cpp
- apps/server/core/AIServer/AIServiceCore.h/.cpp
- apps/server/core/AIServer/AIServiceImpl.h/.cpp
- apps/server/core/AIServer/AIServiceJsonMapper.h/.cpp
- apps/server/core/GateServer/AIServiceClient.h/.cpp
- apps/server/core/GateServer/AIRouteModules.cpp
- apps/client/desktop/MemoChatShared/global.h
- apps/client/desktop/MemoChat-qml/AgentController.h/.cpp
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml
- apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml
- docs/服务通信协议.md

Implementation:
- Added AIOrchestrator `POST /models/api-provider`.
- Added runtime provider persistence at `apps/server/core/AIOrchestrator/.data/api_providers.json` by default.
- Runtime providers are added to `LLMEndpointRegistry.list_endpoints()` and used by the custom OpenAI-compatible client for chat/stream.
- Added Gate route `POST /ai/model/api/register` through AIServer gRPC to AIOrchestrator.
- Added model-settings UI fields for provider name, API base URL, and API Key.
- After registration succeeds, the client refreshes `/ai/model/list` so the new provider models are selectable.

Verification:
- Python compile checks.
- `cmake --build --preset msvc2022-full`.
- AIServer JSON mapper test.

Open risks:
- Live provider calls require a real API key and currently assume OpenAI-compatible `/models` and `/chat/completions`.
- The API key is stored locally in the AIOrchestrator runtime data file; it is not returned by model-list responses.
