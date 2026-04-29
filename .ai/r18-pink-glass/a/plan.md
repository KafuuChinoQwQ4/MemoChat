# Plan

Task summary:
Unify the R18 transition and shell glass colors so the background fades to pale pink and the R18 controls no longer look split into bright/dark halves.

Approach:
- Strengthen the R18 target colors in `GlassBackdrop.qml` so the transition reads as pale pink instead of white glass.
- Update R18-side `GlassSurface` containers in `ChatShellPage.qml` to use stable pale-pink fills and disable repeated backdrop blur sampling.
- Add shared R18 color constants in `R18ShellPane.qml` and apply them consistently to panels, fields, list items, and buttons.
- Verify formatting and run the requested full build.

Files modified:
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/components/GlassBackdrop.qml`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`

Checklist:
- [x] Gather relevant QML context.
- [x] Implement pale-pink backdrop transition.
- [x] Unify R18 panel/control colors.
- [x] Run `git diff --check`.
- [x] Run `cmake --build --preset msvc2022-full`.
- [x] Review final diff and record results.

Assessed: yes
