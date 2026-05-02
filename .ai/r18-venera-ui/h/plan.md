# Plan

Summary: unify MemoChat's chat and R18 sidebars around the R18-style white glass rail, keep a return-to-chat button in R18, and preserve window dragging from both sidebars.

Approach:
- Update `ChatSideBar.qml` defaults to the 72px transparent white/glass icon rail style.
- Update the normal chat sidebar container in `ChatShellPage.qml` to use the same white-glass surface.
- Replace the R18-side old chat sidebar instance with a dedicated R18 navigation rail plus a bottom return-to-chat button.
- Add a drag `MouseArea` to the R18 sidebar surface while keeping button hit areas above it.
- Run QML and client build verification.

Files modified:
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/chat/ChatSideBar.qml`
- `.ai/r18-venera-ui/about.md`
- `.ai/r18-venera-ui/h/*`

Checklist:
- [x] Chat sidebar uses 72px R18-style glass rail sizing and blue-gray hover/selected states.
- [x] R18 sidebar keeps four page buttons: 主页, 本地, 导航, 数据源.
- [x] R18 sidebar has a bottom return-to-chat button wired to `toggleR18Mode()`.
- [x] Normal chat sidebar drag-to-move remains intact.
- [x] R18 sidebar drag-to-move is added.
- [x] Static QML verification completed.
- [x] Client build completed.

Assessed: yes.
