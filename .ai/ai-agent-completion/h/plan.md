# Qwen3 Model List Plan

Assessed: yes

Summary:
- Make the advertised AI model list match the local Ollama model: `qwen3:4b`.

Steps:
- [x] Update AIServer source config default model to `qwen3:4b`.
- [x] Update AIServer runtime config default model to `qwen3:4b`.
- [x] Update AIServer `ListModels` hard-coded Ollama entries to one enabled `qwen3:4b` entry.
- [x] Trim AIOrchestrator source model list to `qwen3:4b` for consistency.
- [x] Build server verification target.
- [x] Restart runtime AIServer and verify Gate `/ai/model/list`.

No database/schema changes.
