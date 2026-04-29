# Qwen3 Model List Context

Task: change the AI model list to `qwen3:4b`.

Relevant files:
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/AIServer/config.ini`
- `infra/Memo_ops/runtime/services/AIServer/config.ini`
- `apps/server/core/AIOrchestrator/config.yaml`

Current state:
- Runtime Ollama tags show only `qwen3:4b` is installed.
- AIOrchestrator source config already uses `qwen3:4b` as default, but also lists unavailable `qwen2.5` variants.
- AIServer config default is still `qwen2.5:7b`.
- AIServer `ListModels` hard-codes `qwen2.5:7b` and `qwen2.5:14b`, so Gate `/ai/model/list` advertises unavailable models.

Docker/services:
- Do not change published ports.
- AIServer runtime is on `8095`.
- Gate HTTP/1 is on `8080`.
- AIOrchestrator is Docker-backed on `8096`.
- Ollama is Docker-backed on `11434`.

Verification:
- Build AIServer through `cmake --build --preset msvc2022-server-verify`.
- Restart only runtime `AIServer` after copying the new binary and updated config.
- Probe `http://127.0.0.1:8080/ai/model/list` through Gate.
