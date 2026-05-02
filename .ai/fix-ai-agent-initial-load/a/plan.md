# Plan

Assessed: yes

- [x] Add idempotent AgentController::ensureInitialized() that refreshes sessions/models once per logged-in uid and can be called safely from QML.
- [x] Call ensureInitialized() when the AI main pane and sidebar pane finish loading/complete, so async Loader timing cannot skip initial data.
- [x] Brighten login frosted glass colors in backdrop, input fields, submit button, and popup surfaces while preserving existing layout.
- [x] Build the client verify preset and review the diff.
