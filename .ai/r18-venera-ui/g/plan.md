# Plan

Summary: Move R18 page navigation to the leftmost R18 sidebar using the newly added icon assets, remove the old unused sidebar, and restore readable gray/dark typography on transparent white glass.

Approach:
- Register the four new PNG icons in `qml.qrc`.
- Replace the R18 face's old `ChatSideBar` with four image navigation buttons.
- Sync R18 sidebar selection with `R18ShellPane.viewMode`.
- Remove the old Canvas navigation rail from `R18ShellPane`.
- Change R18 text/placeholder/error colors to gray/dark colors.
- Verify QML and client build.

Checklist:
- [x] Add qrc aliases for new R18 icons.
- [x] Move R18 navigation to ChatShellPage's left rail.
- [x] Remove old R18 sidebars/Canvas icon navigation.
- [x] Change R18 typography to gray/dark tokens.
- [x] Run qmllint, diff check, and client verify build.

Assessed: yes
