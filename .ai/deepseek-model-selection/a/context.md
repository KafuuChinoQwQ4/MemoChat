# Context

Relevant files:
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`: model cache, switchModel, model list handling, chat/stream payload model fields.
- `apps/client/desktop/MemoChat-qml/AgentController.h`: private controller state.
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`: model list delegates call `switchModel(model_type, model_name)`.
- `apps/server/core/AIOrchestrator/harness/llm/service.py`: provider ids include runtime `api-deepseek`.

Runtime observations:
- `/models` currently returns DeepSeek provider `api-deepseek` with `deepseek-v4-flash` and `deepseek-v4-pro`.
- `/models` default_model remains `ollama:qwen3:4b`.

Risk:
- We should not override a user who explicitly selected another model. Add a persisted explicit selection flag.
- First-time existing users may have a cached Ollama selection from the old default; if there is an API provider available, choosing it is likely what they intended after registration.
