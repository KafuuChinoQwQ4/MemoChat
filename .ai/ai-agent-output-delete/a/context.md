# Context

Task:
The AI Agent screen does not output answers, sessions cannot be deleted cleanly, and asking questions shows `error=2`.

Findings:
- Docker AI dependencies are running. `memochat-ai-orchestrator` is healthy on `8096`, and Ollama on `11434` returns `qwen3:4b`.
- No local `GateServer`, `AIServer`, `ChatServer`, or `MemoChatQml` process was visible during inspection, so runtime smoke needs service startup.
- Client `ErrorCodes::ERR_NETWORK` is `2`.
- The composer used non-streaming `sendMessage`, which goes through `HttpMgr` with a 10 second timeout. LLM responses often exceed that.
- The streaming path only updated the local message if server `msg_id` matched the client placeholder id. They are different ids, so chunks could arrive without appearing.
- Qt can report `QNetworkReply::RemoteHostClosedError` value `2` when an SSE response closes after useful data. The client treated that as a hard failure and overwrote the response with an error.
- Deleting the current session did not clear `_current_session_id` or the message model.

Relevant files:
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChatShared/httpmgr.cpp`
- `apps/server/core/GateServer/AIRouteModules.cpp`
- `apps/server/core/AIServer/AIServiceCore.cpp`

Docker/MCP:
- Used `docker ps`.
- Used direct HTTP probes for AIOrchestrator `/health` and Ollama `/api/tags`.
