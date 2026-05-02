# Context

Task:
- Improve math formatting in rendered AI Markdown.
- Fix top-level Memory and Task panes showing AI service errors.

Findings:
- Screenshot shows raw LaTeX inline math like `\(u\)` and `\(c\)` appearing in rendered output.
- `MarkdownRenderer.cpp` previously handled Markdown but not LaTeX math delimiters.
- Desktop memory/task calls route through Gate:
  - `/ai/memory`
  - `/ai/tasks`
- Gate forwards to AIServer, and AIServer forwards to AIOrchestrator:
  - `/agent/memory`
  - `/agent/tasks`
- Runtime check against `http://127.0.0.1:8096/openapi.json` showed the running `memochat-ai-orchestrator` container does not expose `/agent/memory` or `/agent/tasks`, despite local code having those routes.
- Docker logs show `GET /agent/memory?... 404` and `GET /agent/tasks?... 404`.

Conclusion:
- The service error is caused by the running Docker image/container being older than the local AIOrchestrator code.
- Rebuild/restart AIOrchestrator after the code changes are in place.

Verification:
- Python compile for AIOrchestrator entry/router passed.
- `tests.test_harness_structure` passed.
- Client configure/build passed.
