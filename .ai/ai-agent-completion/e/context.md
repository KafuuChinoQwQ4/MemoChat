# AI Agent Streaming Trace Context

Task: complete the remaining AI Agent trace work by carrying orchestration, memory, execution, and feedback layer events through the streaming chat path.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/common/proto/ai_message.proto`
- `apps/server/core/AIServer/AIServiceClient.h`
- `apps/server/core/AIServer/AIServiceClient.cpp`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/GateServer/AIServiceClient.h`
- `apps/server/core/GateServer/AIServiceClient.cpp`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`

Current state:
- Non-streaming `/ai/chat` now returns `trace_id`, `skill`, `feedback_summary`, `observations`, and `events`.
- The desktop trace panel reads those fields from `AgentController`.
- Streaming `/ai/chat/stream` only forwards text chunks and basic token metadata through C++.

Dependencies:
- Docker infrastructure is running. AIOrchestrator remains on host port `8096`; no port changes are needed.
- No database or persistent schema change is required.

Open risks:
- Streaming C++ callback signatures are shared between AIServer and GateServer. All call sites must be updated together.
- Runtime stream verification depends on local Ollama response speed.
