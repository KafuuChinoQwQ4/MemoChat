# Stage 5 Client Memory Completion Context

Task: continue building the MemoChat AI Agent by making the visible memory API usable from the desktop client.

Relevant existing state:
- AIOrchestrator already exposes `GET /agent/memory?uid=&limit=` and `DELETE /agent/memory/{memory_id}?uid=`.
- Desktop QML calls Gate routes under `/ai/...`, so it cannot use `/agent/memory` directly.
- Existing knowledge-base path is the local pattern to follow:
  - Gate HTTP routes in `apps/server/core/GateServer/AIRouteModules.cpp`
  - Gate gRPC client in `apps/server/core/GateServer/AIServiceClient.*`
  - AIServer gRPC service/core/client in `apps/server/core/AIServer/*`
  - proto contract in `apps/server/core/common/proto/ai_message.proto`
  - desktop controller in `apps/client/desktop/MemoChat-qml/AgentController.*`
  - QML entry in `AgentPane.qml`, `ChatShellPage.qml`, and `qml.qrc`

Services and ports:
- No Docker port/config changes.
- AIOrchestrator remains on port `8096`.
- AIServer gRPC remains on port `8095`.
- Gate HTTP/HTTPS ports remain unchanged.

Risks:
- Proto changes require regenerating generated gRPC files through the existing CMake build.
- The desktop client uses a generic HTTP manager without DELETE, so Gate should expose a POST delete route matching existing `/ai/kb/delete` behavior.
- The current worktree is dirty with previous project work; do not revert unrelated edits.

Verification planned:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- `cmake --preset msvc2022-client-verify`
- `cmake --build --preset msvc2022-client-verify`
