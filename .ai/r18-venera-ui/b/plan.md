# Plan

Task summary: add real Venera official source catalog fetching and JS source staging/import to MemoChat R18, without pretending the Venera JS execution runtime exists yet.

Implementation:

- Add a QML-facing `officialSourceModel` to `R18Controller`.
- Add controller methods:
  - `refreshOfficialSources(url)` downloads Venera's official `index.json`.
  - `importOfficialSource(row)` resolves `url` or relative `fileName`/`filename`, downloads the JS script, and sends it to `/api/r18/source/import` with a manifest.
- Update server import route to accept `.js` source payloads in addition to zip, persist them as `source.js`, and mark them as `staged-js`.
- Update `R18ShellPane.qml` source management page with an official catalog URL field, load button, source list, and import button per catalog row.
- Update docs and `.ai` logs to distinguish catalog/script staging from real executable source runtime.

Files:

- `apps/client/desktop/MemoChat-qml/R18Controller.h`
- `apps/client/desktop/MemoChat-qml/R18Controller.cpp`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `apps/server/core/GateServerHttp1.1/H1R18Routes.cpp`
- `docs/当前架构基准.md`
- `docs/API参考.md`
- `.ai/r18-venera-ui/b/*`

Completion definition:

- Official Venera catalog can be downloaded into QML model.
- A selected catalog source can be downloaded and sent to server import route as JS content.
- Server persists JS source metadata and lists it as staged.
- Docs explain staged JS sources still require a later JS runtime adapter before real search/detail/pages come from those sources.
- Client and server verify builds pass, or blocking failures are recorded.

Status:

- [x] Context gathered.
- [x] Plan drafted.
- [x] Assessed: yes.
- [x] Add client catalog fetch/import.
- [x] Add source-management UI.
- [x] Extend server import for JS staging.
- [x] Update docs.
- [x] Verify.
- [x] Review.

Verification result:

- `Invoke-WebRequest -UseBasicParsing -Uri "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/index.json"` returned the official Venera source index with source entries.
- `Invoke-WebRequest -UseBasicParsing -Uri "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/nhentai.js"` returned a JavaScript source script.
- `git diff --check` for touched files exited 0 with LF/CRLF warnings only.
- `qmllint apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml` exited 0 with existing unqualified delegate access warnings.
- `cmake --preset msvc2022-client-verify` exited 0.
- `cmake --build --preset msvc2022-client-verify` exited 0 after fixing lambda captures in `R18Controller.cpp`.
- `cmake --preset msvc2022-server-verify` exited 0 with existing CMake dev/deprecation warnings.
- `cmake --build --preset msvc2022-server-verify` exited 0 and linked `GateServerHttp1.1.exe`.
