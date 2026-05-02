User asked to fix model selection because they are using the DeepSeek API, but the failed request went through Ollama. Start: 2026-05-01T01:41:18.2766735-07:00.

Known context:
- AIOrchestrator `/models` returns `api-deepseek` models and `ollama:qwen3:4b`; default_model is still Ollama.
- Frontend `AgentController` initializes current model to Ollama and sends `_current_model_backend/_current_model_name` in chat and stream requests.
- `handleModelListRsp` falls back to backend default when the current cached model is unavailable. This can keep users on Ollama even after registering an API provider unless they explicitly click the API model.

Goal: prefer the API provider after registration/refresh when the current model is the built-in Ollama default, while preserving explicit user model choices.
