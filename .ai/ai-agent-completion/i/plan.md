# Plan

Task summary: finish backend AI Agent model discovery and knowledge-base list/delete forwarding.

Approach:

- Add narrowly scoped HTTP GET and DELETE JSON helpers to AIServer AIServiceClient.
- Add AIServer client methods for `/models`, `/kb/list`, and `/kb/{kb_id}`.
- Map AIOrchestrator JSON results into existing protobuf fields in AIServiceCore.
- Keep fallback model list from config for resilience if AIOrchestrator is unavailable.

Files to modify:

- `apps/server/core/AIServer/AIServiceClient.h`
- `apps/server/core/AIServer/AIServiceClient.cpp`
- `apps/server/core/AIServer/AIServiceCore.cpp`
- `.ai/ai-agent-completion/i/*`

Implementation checklist:

- [x] Setup task artifact folder.
- [x] Gather context and validate route/proto shape.
- [x] Assess plan against actual symbols.
- [x] Add GET/DELETE helpers and URL encoding.
- [x] Add `ListModels`, `KbList`, `KbDelete` client wrappers.
- [x] Replace AIServer Core hardcoded/placeholders with forwarded results plus fallback.
- [x] Build/runtime verify.
- [x] Review diff and record results.

Assessed: yes
