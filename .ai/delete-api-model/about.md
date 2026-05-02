# Delete API Models

MemoChat lets users remove API providers that were added from the AI model settings panel. Runtime API providers are persisted by AIOrchestrator and appear in the model list as `api-*` model types. The desktop UI shows a compact rounded delete action beside those models, calls Gate/AIServer/AIOrchestrator, then refreshes the model list and falls back to an available model if the current one was removed.
