# Plan

Summary: Fix frontend model selection so registered API providers are not bypassed by the Ollama default.

Steps:
- [x] Add a persisted `_model_selection_explicit` flag to `AgentController`.
- [x] Mark the flag true in `switchModel` because it is user initiated from the UI.
- [x] In `handleModelListRsp`, if current model is unavailable use default fallback as before; if current model is still automatic/default and an enabled `api-*` model exists, switch to the first API model.
- [x] After successful API provider registration, switch immediately to the first returned model and mark it explicit.
- [x] Build client verify.

Assessed: yes. This is a client-only behavior fix. No database or Docker port changes needed.
