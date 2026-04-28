# Plan

Assessed: yes

Task summary:
- Eliminate the crash path in `GateServer` and harden `ChatServer` shutdown/lifetime handling.

Approach:
1. Fix `GateServer` HTTP/2 header value lifetime so nghttp2 never sees dangling string pointers.
2. Harden `ChatServer` accept/timer/session teardown to stop callbacks from running after shutdown.
3. Rebuild the server preset and restart local services.
4. Re-run a short runtime smoke check and observe for delayed crashes.

Files changed:
- `apps/server/core/GateServer/NgHttp2Server.cpp`
- `apps/server/core/GateServer/GateServer.cpp`
- `apps/server/core/ChatServer/ChatIngressCoordinator.cpp`
- `apps/server/core/ChatServer/CServer.cpp`
- `apps/server/core/ChatServer/CServer.h`
- `apps/server/core/ChatServer/CSession.cpp`
- `apps/server/core/ChatServer/CSession.h`

Implementation phases:
- Phase 1: Replace temporary header buffers with owned strings and remove detached HTTP/2 thread shutdown risk.
- Phase 2: Add explicit ChatServer start/stop gating and detach session back-pointers on shutdown.
- Phase 3: Build and redeploy the server preset.
- Phase 4: Restart services and run smoke checks.

Docker/MCP checks:
- Confirm core Docker dependencies stay healthy.
- No port changes.

Build/test commands:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- `tools\scripts\status\stop-all-services.bat`
- `tools\scripts\status\start-all-services.bat`
- `tools\scripts\test_register_login.ps1`

Status checklist:
- [x] Gather logs and reproduce
- [x] Identify crash sources
- [x] Patch code
- [x] Rebuild successfully
- [x] Restart services
- [x] Observe for delayed crash
