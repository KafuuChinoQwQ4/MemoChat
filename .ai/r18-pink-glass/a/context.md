# Context

Task:
When switching to R18, the white transparent frosted background should fade into pale pink. The upper controls currently look uneven, with a visible bright/dark split, and should be more unified.

Screenshot summary:
- R18 mode shows a pale pink shell, but the surrounding/base glass still reads gray-white.
- The main content panel has a vertical split where the left side is brighter and the right side is darker.
- Left controls and main panel colors are close but not unified.

Relevant files:
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/components/GlassBackdrop.qml`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/components/GlassSurface.qml`

Implementation notes:
- This is a QML-only visual change. No Docker containers, database schema, queues, or stable ports are touched.
- Existing user changes are present across the repo; only the relevant QML files are intentionally edited.
- To reduce the split-tone artifact, R18-specific `GlassSurface` layers use stable translucent pale-pink fills instead of sampling the backdrop repeatedly.

Verification:
- `git diff --check` for touched QML files.
- User requested full build: `cmake --build --preset msvc2022-full`.
