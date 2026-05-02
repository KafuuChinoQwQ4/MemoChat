# Context

Task: user clarified that the previous R18 work only built a client shell; real source fetching should use Venera's official comic source repository.

Start time: 2026-05-02T00:15:59.1813234-07:00.

Current state from previous task:

- R18 QML has a Venera-inspired shell: bookshelf/search/history/source/detail/reader.
- `R18Controller` can call source/history/search/detail/pages/favorite/progress routes.
- Server R18 routes still return mock search/detail/pages and source import only accepts zip payloads.

Official Venera source repository:

- Default URL from Venera code/docs: `https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/index.json`.
- Confirmed live response on 2026-05-02. The index is an array of entries with fields such as `name`, `fileName`, `key`, `version`, and optional `description`; examples include `copy_manga.js`, `picacg.js`, `nhentai.js`, `ehentai.js`, `jm.js`, `manga_dex.js`, etc.
- Venera docs state comic sources are JavaScript, executed by Venera's `flutter_qjs` runtime. The source list can specify `url` or `filename`/`fileName`; relative files resolve beside the index URL.

Important boundary:

- This task should not claim full real search/reading until MemoChat has a JS source runtime compatible with Venera's JS APIs (`Network`, `Comic`, source class parser, image loading hooks, etc.).
- This task can implement real official source catalog fetching and real JS source script import/staging, so the server/client no longer only accept zip placeholders.

Relevant files:

- `apps/client/desktop/MemoChat-qml/R18Controller.{h,cpp}`: add official catalog model and download/import methods.
- `apps/client/desktop/MemoChat-qml/R18ListModel.{h,cpp}`: already has generic roles and `count`.
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: source management UI.
- `apps/server/core/GateServerHttp1.1/H1R18Routes.cpp`: source import and source list service.
- `docs/当前架构基准.md`, `docs/API参考.md`, `docs/架构文档.md`: docs must clarify staged Venera JS sources versus executable source runtime.

Build/test commands:

- Client: `cmake --preset msvc2022-client-verify`; `cmake --build --preset msvc2022-client-verify`.
- Server route changes: `cmake --preset msvc2022-server-verify`; `cmake --build --preset msvc2022-server-verify`.

Risks:

- Existing worktree has many unrelated user changes. Keep edits scoped.
- Live official catalog fetching depends on network availability; verification can use direct `Invoke-WebRequest` plus build if runtime services are not started.
- Server import route is in `GateServerHttp1.1`, while main local GateServer route availability may depend on current deployment target.

