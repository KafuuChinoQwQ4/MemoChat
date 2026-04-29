# Context

Task: continue AI Agent completion after backend regression/recovery hardening. Close the two remaining high-value gaps:

- unify AIOrchestrator runtime config and image behavior so container rebuilds do not drift back to stale model inventory or implicit config state
- finish Agent / KnowledgeBase frontend exception-state handling and obvious UI coordination gaps

Relevant files:

- `apps/server/core/AIOrchestrator/config.py`
- `apps/server/core/AIOrchestrator/config.yaml`
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `apps/server/core/AIOrchestrator/Dockerfile`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/KnowledgeBasePane.qml`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`

Current findings:

- AIOrchestrator image copies repository `config.yaml` into `/app/config.yaml`, but runtime behavior still depends on a separate set of environment overrides. The active config source is implicit rather than explicit.
- The current container already has the updated qwen3-only config inside `/app/config.yaml`, but that was previously corrected by manually copying the file into the container. That is a process smell and should be eliminated.
- AIOrchestrator compose currently:
  - mounts cache directories under the repo working tree (`./.cache/...`)
  - waits on dependency startup via plain `depends_on` instead of health-gated conditions
  - does not mount the runtime config file explicitly
- Client-side Agent defaults still start at `qwen2.5:7b` in `AgentController.cpp`, even though the current backend default is `qwen3:4b`.
- `KnowledgeBasePane.qml` triggers `reloadRequested()` immediately after upload/delete button clicks, before the async backend operation has finished.
- `AgentController::error()` exposes `_error`, but `_error` is not actually updated before `errorOccurred` is emitted, so the bound QML error banner can stay empty.
- KB modal state is incomplete:
  - no dedicated busy/error/status props for KB list/search/upload/delete
  - no search-empty feedback
  - search result area uses a plain `Label` with an invalid `ScrollBar.vertical` attachment
- Model modal state is also thin:
  - no busy/empty state for model refresh
  - opening the modal does not proactively refresh the available model list

Docker / runtime checks used:

- `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"`
- `docker inspect memochat-ai-orchestrator --format "{{json .Config.Env}}"`
- `docker exec memochat-ai-orchestrator sh -lc "ls -la /app && sed -n '1,220p' /app/config.yaml"`

Build / verification plan:

- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest tests.test_ollama_recovery tests.test_harness_structure`
- `cmake --preset msvc2022-client-verify`
- `cmake --build --preset msvc2022-client-verify`
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- `docker compose -f apps/server/core/AIOrchestrator/docker-compose.yml config`
- if safe, `docker compose -f apps/server/core/AIOrchestrator/docker-compose.yml up -d --build memochat-ai-orchestrator`

Open risks:

- Switching the Ollama model-data bind mount away from the existing repo-local path could strand the currently pulled model set. Avoid data-path churn unless migration is deliberate.
- Client build may surface pre-existing QML warnings unrelated to the touched Agent pages; review carefully before attributing them to this phase.
