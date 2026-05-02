# Plan

Summary: Align the R18 interface with the MemoChat chat glass/acrylic visual language and remove the pink-heavy R18 tint.

Approach:
- Reuse the chat UI translucent white glass token family in R18ShellPane.
- Keep the R18 flip behavior but disable the pink acrylic tint path for the shared backdrop/window.
- Replace R18 face sidebar pink mix colors with the chat sidebar blue-gray colors.
- Verify QML syntax and client build.

Files:
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml
- apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml
- .ai/r18-venera-ui/f/*

Checklist:
- [x] Inspect chat glass color references.
- [x] Replace R18 pink panel/button/text tokens with chat-like glass tokens.
- [x] Remove R18 shared backdrop/window pink tint.
- [x] Run qmllint and diff check.
- [x] Run client verify build.
- [x] Review final diff and record result.

Assessed: yes

