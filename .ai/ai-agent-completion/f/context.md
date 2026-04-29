# Runtime Update Restart Context

Task: update runtime services with the latest verified AI Agent trace binaries and restart the local services.

Relevant runtime pieces:
- Latest verified server binaries: `build-verify-server/bin/Release`
- Latest verified client binary: `build-verify-client/bin/Release`
- Runtime deploy source expected by script: `build/bin/Release`
- Runtime service dir: `infra/Memo_ops/runtime/services`
- Deploy script: `tools/scripts/status/deploy_services.bat`

Current state before restart:
- No `GateServer`, `AIServer`, `ChatServer`, `StatusServer`, `VarifyServer`, or `MemoChat` process was detected.
- Docker dependencies are running and ports are unchanged.
- `memochat-ai-orchestrator` is already updated and running on `8096`.

Plan:
- Copy verified binaries into `build/bin/Release`.
- Run deploy script to refresh runtime service directories.
- Start `AIServer` and `GateServer1` from runtime with hidden service runner processes.
- Probe ports and AI routes.

Follow-up fixes applied during runtime smoke:
- Gate POST routes were returning empty responses because connection deadlines were not reset per request; `HttpConnection::CheckDeadline()` now resets the timer and the HTTP deadline was widened to 600 seconds for slow local LLM calls.
- `Singleton::GetInstance(args...)` and `GetInstance()` could construct different instances because each overload had its own `once_flag`; the shared instance is now guarded by one mutex path.
- Gate/AIServer JSON bool and missing-field reads now default safely, fixing missing `is_final` and `bad variant access` failures.
- Gate AI JSON nested fields (`session`, `default_model`) now build child objects explicitly instead of relying on unsupported chained proxy writes.
- AIServer AIOrchestrator HTTP client now uses configured Beast stream timeout.
- AIServer session repository serializes pqxx access to avoid overlapping transactions from concurrent gRPC calls.

Runtime verification highlights:
- Docker dependencies stayed up on stable ports.
- `GateServer` is listening on `8080`, `AIServer` on `8095`, AIOrchestrator on `8096`.
- `/ai/model/list` returns a structured `default_model`.
- `/ai/session` returns a structured `session`.
- `/ai/chat` returns `code=0` with `trace_id`, `skill`, `feedback_summary`, `observations`, and multi-layer `events`.
- `/ai/chat/stream` was verified with a short request and final SSE contained `is_final=true` plus trace events.
