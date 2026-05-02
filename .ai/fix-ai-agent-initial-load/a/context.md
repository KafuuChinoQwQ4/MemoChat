# Context

The initial AI tab load depends on AppController::switchChatTab calling AgentController::loadSessions() and refreshModelList() only when chatTab changes. The chat shell and AI panes are created asynchronously by QML Loader, so first creation can happen after the C++ trigger. ChatLeftPanel also refreshes only on currentTab changes. A QML-side loaded/completed call into an idempotent controller initializer is the narrowest fix.

Login page uses GlassBackdrop plus GlassTextField/LoginSubmitButton/LoginMoreOptionsPopup colors. Defaults are translucent and can read gray against the pale backdrop. Brighten backdrop and glass fills/strokes rather than changing layout.

## Verification target
Run client CMake configure/build if available: cmake --preset msvc2022-client-verify; cmake --build --preset msvc2022-client-verify.
