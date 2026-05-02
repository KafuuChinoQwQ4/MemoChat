# Plan

Summary: change R18 source management so users can directly pull a comic source JavaScript URL such as `jm.js`, matching how Venera stores and updates individual sources.

Implementation:

- Add `R18Controller::importSourceUrl(url)` that downloads a direct `.js` or package URL and imports it through `/api/r18/source/import`.
- Reuse existing source-script import logic by factoring script download completion into a helper.
- Update `R18ShellPane.qml` source management:
  - Primary field defaults to `https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/jm.js`.
  - Primary button text becomes `拉取JM源`.
  - Catalog section becomes optional `从目录选择`.
- Update docs and `.ai` records to clarify direct source URL import is primary.

Files:

- `apps/client/desktop/MemoChat-qml/R18Controller.h`
- `apps/client/desktop/MemoChat-qml/R18Controller.cpp`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `docs/当前架构基准.md`
- `docs/架构文档.md`
- `.ai/r18-venera-ui/c/*`

Status:

- [x] Context gathered.
- [x] Plan drafted.
- [x] Assessed: yes.
- [x] Add direct source URL import.
- [x] Update QML primary source UI.
- [x] Update docs.
- [x] Verify.
- [x] Review.

Verification evidence:

- `Invoke-WebRequest -UseBasicParsing -Uri "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/jm.js"` returned a JavaScript source payload.
- `git diff --check` on touched R18/client/server/doc files exited 0, with only existing LF/CRLF normalization warnings.
- `qmllint apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml` exited 0; it reported existing Qt unqualified-access warnings.
- `cmake --build --preset msvc2022-client-verify` exited 0 and linked `bin\Release\MemoChatQml.exe`.
- Previous server verification for JS staging passed with `cmake --preset msvc2022-server-verify` and `cmake --build --preset msvc2022-server-verify`.

Review outcome:

- Direct source URL import is now the primary UI path.
- Venera catalog import remains optional discovery.
- The remaining functional gap is executing staged Venera JavaScript sources for real search/detail/page resolution.
