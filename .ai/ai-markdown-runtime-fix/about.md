# AI Markdown And Runtime Fix

MemoChat renders common inline math in AI Markdown replies and keeps AI Agent memory/task entry points aligned with the AIOrchestrator harness routes. Runtime service checks showed the currently running AIOrchestrator container is still serving an older OpenAPI schema without `/agent/memory` and `/agent/tasks`; code is prepared, but the container image must be rebuilt/restarted for those routes to be available at runtime.
