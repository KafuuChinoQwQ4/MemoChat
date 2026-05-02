# Context

Task: continue borrowing ideas from `https://github.com/venera-app/venera.git` and adapt them into MemoChat's R18 new UI.

Start time: 2026-05-01T23:58:23.1010348-07:00.

Relevant MemoChat files:

- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: current R18 QML shell with search, detail, reader, and source management.
- `apps/client/desktop/MemoChat-qml/R18Controller.{h,cpp}`: QML-facing controller for `/api/r18/*` HTTP calls.
- `apps/client/desktop/MemoChat-qml/R18ListModel.{h,cpp}`: generic list roles used by source/comic/chapter/page models.
- `apps/client/desktop/MemoChat-qml/qml.qrc`: registers `qml/r18/R18ShellPane.qml`.
- `apps/server/core/GateServerHttp1.1/H1R18Routes.cpp`: existing R18 API routes. It exposes sources, import, search, detail, pages, favorite toggle, history update, history list, and image.
- `docs/当前架构基准.md`: architecture baseline documents R18 as isolated client UI + gateway routes + C++ plugin protocol.

Venera reference checked:

- Cloned shallow to `D:\codex-cache\venera`, latest checked commit `a0eba91 Update README.md`.
- `README.md` says Venera is a comic reader for local/network comics with JavaScript comic sources, favorites, downloads, tags/comments, and source-supported login.
- `lib/pages/main_page.dart`: top-level navigation uses Home, Favorites, Explore, Categories with Search/Settings actions.
- `lib/pages/home_page.dart`: home aggregates search, sync, history, local comics, updates, source list, and image favorites.
- `lib/pages/comic_source_page.dart`: source management supports URL/config-file import, update checks, edit/update/delete, and per-source settings/account state.
- `lib/pages/reader/scaffold.dart`: reader uses overlay top/bottom bars, page status, chapter drawer, favorite/download/share/settings actions.

Adaptation decisions:

- Borrow structure and interaction ideas, not Venera implementation code, Flutter widgets, or third-party source scripts.
- Keep MemoChat's R18 API boundary and plugin-source architecture intact.
- Add QML-visible history/favorite/progress calls because the gateway already exposes matching routes.
- Keep UI styling consistent with existing pale-pink glass R18 shell.

Docker/MCP state:

- `docker ps` shows required infrastructure containers running with stable ports, including Postgres `15432`, Redis `6379`, Mongo `27017`, MinIO `9000/9001`, Redpanda `19092`, RabbitMQ `5672/15672`, Ollama `11434`, Qdrant `6333/6334`, Neo4j `7474/7687`, Prometheus `9090`, Grafana `3000`, Loki `3100`, Tempo `3200`, InfluxDB `8086`, cAdvisor `8088`, and AIOrchestrator `8096`.
- No database schema change is planned.

Build/test commands:

- Preferred client verification: `cmake --preset msvc2022-client-verify` and `cmake --build --preset msvc2022-client-verify`.
- If configure is already current, at minimum run `cmake --build --preset msvc2022-client-verify`.

Cold-start answers:

- System: Windows Qt/QML MemoChat client plus C++ gateway services and Docker-only infrastructure.
- Current task: adapt Venera-like comic reader structure into the MemoChat R18 QML shell.
- Related code/config: QML and R18 controller files listed above; no Docker port changes.
- Current step: context gathered, plan next, then implement one UI/controller stage.
- Completion evidence: client build passes, `.ai` logs contain commands/results, docs/artifacts note the Venera adaptation and no schema/config drift.

Risks:

- Existing worktree is very dirty with many user changes. This task must avoid reverting unrelated files.
- Runtime screenshot may not be available if the local UI app or login state cannot be launched quickly; build verification is still required.
- Gateway history/favorite endpoints are placeholders today, so the UI must handle empty history and echoed favorite state gracefully.

Context budget:

- Loaded the main MemoChat skill, `skills/qml-ui.md`, `skills/task.md`, current R18 QML/controller/model files, R18 server routes, `qml.qrc`, Docker status, and focused Venera files.
- Did not load all Venera sources or every MemoChat UI component to avoid noise.

