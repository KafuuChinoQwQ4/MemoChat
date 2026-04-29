# Context

Task: continue AI Agent completion after stream output and Qwen3 4B model list work. Close the remaining backend gaps for AI Agent output:

- AIServer should forward model list discovery to AIOrchestrator `/models` instead of hardcoding.
- AIServer should implement knowledge-base list/delete by forwarding to AIOrchestrator `/kb/list` and `DELETE /kb/{kb_id}`.
- Preserve existing dirty-tree work and stable Docker ports.

Relevant files:

- `apps/server/core/AIServer/AIServiceClient.h`
- `apps/server/core/AIServer/AIServiceClient.cpp`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/AIOrchestrator/api/model_router.py`
- `apps/server/core/AIOrchestrator/api/kb_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/common/proto/ai_message.proto`
- `apps/server/core/GateServer/AIServiceClient.cpp`

Current findings:

- Gate routes and gRPC client already call AIServer `KbList` and `KbDelete`.
- AIServer `ListKb` and `DeleteKb` currently return success without contacting AIOrchestrator.
- AIServer `AIServiceClient` has `PostJson` but no GET/DELETE helper.
- AIOrchestrator exposes `GET /models`, `GET /kb/list?uid=...`, and `DELETE /kb/{kb_id}?uid=...`.
- AIOrchestrator model response includes provider metadata, but protobuf `ModelInfo` only supports model type/name/display/enabled/context window.

Docker/services:

- `memochat-ai-orchestrator` on `8096`
- `memochat-ollama` on `11434`
- `memochat-qdrant` on `6333/6334`
- `memochat-neo4j` on `7474/7687`
- `memochat-postgres` on `15432`

Verification plan:

- `python -m compileall apps\server\core\AIOrchestrator`
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- Runtime probes if build output can be used without file locks:
  - `http://127.0.0.1:8096/models`
  - `http://127.0.0.1:8080/ai/model/list`
  - `http://127.0.0.1:8080/ai/kb/list?uid=900001`
