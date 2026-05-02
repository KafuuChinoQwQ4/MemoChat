# Plan

Task summary: adapt Venera-inspired comic-reader organization into MemoChat's R18 QML shell while preserving MemoChat's isolated R18 API/plugin boundary.

Implementation approach:

- Extend `R18Controller` with QML-visible history, favorite toggle, and history update calls using existing `/api/r18/history`, `/api/r18/favorite/toggle`, and `/api/r18/history/update`.
- Rework `R18ShellPane.qml` into a Venera-inspired layout: left navigation, bookshelf home, search/results, history, source management, detail, and reader with top/bottom controls.
- Keep colors and controls aligned with existing R18 pale-pink glass style.
- Update docs/task artifacts to record that this is a product-structure adaptation, not a direct dependency on Venera source code.

Files to modify:

- `apps/client/desktop/MemoChat-qml/R18Controller.h`
- `apps/client/desktop/MemoChat-qml/R18Controller.cpp`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `docs/当前架构基准.md`
- `.ai/r18-venera-ui/a/*`

Document sync:

- Update the architecture baseline R18 section to mention the Venera-inspired bookshelf/search/history/source/reader UI and preserve the no-hardcoded-third-party-source boundary.

Required Docker/MCP checks:

- `docker ps` before implementation already confirmed stable infrastructure containers and ports.
- No DB writes or migrations are needed.

Build/test commands:

- `cmake --preset msvc2022-client-verify`
- `cmake --build --preset msvc2022-client-verify`

Definition of done:

- R18 controller exposes history and favorite/progress operations to QML.
- R18 shell compiles and shows coherent states for bookshelf/search/detail/reader/sources/history.
- Client build passes, or failure is logged with the blocking reason.
- Documentation and `.ai` artifacts are updated.

Three-layer gates:

1. Static/build: client CMake configure/build.
2. Behavior: source-level review of R18 API operations and QML state transitions.
3. Runtime/UI: manual visual checklist recorded if automated screenshot is unavailable.

WIP scope:

- In scope: QML R18 shell and its existing C++ controller.
- Out of scope: adding real third-party source scripts, Venera JS compatibility, persistent favorite/history storage, database migrations, or changing Docker ports.

Status:

- [x] Gather context.
- [x] Draft plan.
- [x] Assessed: yes.
- [x] Extend controller.
- [x] Rework R18 QML shell.
- [x] Update docs.
- [x] Verify build.
- [x] Review diff.

Verification result:

- `qmllint apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: exit 0 with unqualified-access warnings only.
- `cmake --preset msvc2022-client-verify`: exit 0, optional Vulkan header warning only.
- `cmake --build --preset msvc2022-client-verify`: exit 0, rebuilt R18 files, QML resource, and `MemoChatQml.exe`.
