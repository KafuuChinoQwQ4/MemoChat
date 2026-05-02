# Plan

Task summary: allow removing API-connected models from the AI model list.

Approach:
- Treat `api-*` model types as runtime API providers.
- Delete the provider by `provider_id/model_type`; this removes all models discovered from that API provider.
- Keep built-in/config providers non-deletable in the UI.

Steps:
- [x] Gather context
- [x] Assess route and model-list contracts
- [ ] Add AIOrchestrator runtime provider delete API
- [ ] Add AIServer and Gate bridge methods
- [ ] Add AgentController invokable and response handling
- [ ] Add rounded delete action in model list UI
- [ ] Verify server/client builds and Python checks
- [ ] Review diff

Assessed: yes
