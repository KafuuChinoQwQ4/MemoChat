# Context

Task: user reported that the R18 UI text was blurry / ghosted and the overall pink tone was too heavy. User provided screenshots: the first screenshot shows the current pink glass UI with white text losing clarity; the following four screenshots show Venera's cleaner white UI with a narrow left sidebar and four primary page buttons.

Start time: 2026-05-02T00:53:41.4938544-07:00.

Relevant files:

- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: main visual update.
- `apps/client/desktop/MemoChat-qml/qml.qrc`: currently references `AgentMemoryPane.qml` and `AgentTaskPane.qml`; these files were restored from stash to keep the client build green.
- `apps/client/desktop/MemoChat-qml/R18Controller.*` and `apps/server/core/GateServerHttp1.1/H1R18Routes.cpp`: carried forward from the direct JM source import work.

Design correction:

- Reduce pink glass styling to a mostly white surface with pale blue selected states.
- Use dark text colors instead of white text over pale pink translucent panels.
- Replace the wide left panel with a narrow Venera-like sidebar.
- Keep four primary page buttons: shelf, search, history, and comic sources.
- Keep detail and reader as drill-in subpages rather than primary sidebar buttons.

Verification:

- `qmllint apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: exit 0, with existing unqualified-access warnings only.
- `git diff --check` on touched R18/client/server/doc files: exit 0, LF/CRLF warnings only.
- `cmake --build --preset msvc2022-client-verify`: exit 0, linked `bin\Release\MemoChatQml.exe`.

Residual notes:

- No automated screenshot runner was used in this turn; the visual target is based on the user-provided screenshots.
- The remaining `qmllint` warnings are the existing delegate ID/model qualification style warnings.
