---
description: Create or adapt SVG/icon assets for MemoChat QML and ops UI without external vectorization services unless the user explicitly provides one.
---

# MemoChat Icon

Use for project UI icon work in QML/client/ops surfaces.

## Inputs

Collect:

- target surface: MemoChat QML client, MemoOps QML, web/ops asset, or docs
- intended meaning and state
- target size
- color behavior: fixed color, theme color, or mask/tint
- destination path
- screenshot or source asset path if provided

If the user pasted an image but no file path exists, summarize the visual request and ask for a saved file path only when precise tracing is required.

## Search Existing Assets

Before creating anything, inspect:

- `apps/client/desktop/MemoChat-qml`
- `apps/client/desktop/MemoChatShared`
- `infra/Memo_ops/client/MemoOps-qml`
- nearby `assets`, `icons`, `resources`, `qml.qrc`, or CMake resource entries

Prefer existing style, naming, size, and resource registration patterns.

## SVG Rules

- Keep SVG minimal and hand-editable.
- Use `viewBox` correctly; do not rely on arbitrary transforms when simple coordinates work.
- Prefer `currentColor` or a single fill when the QML component tints icons.
- Avoid embedded raster images unless the design requires them.
- Avoid metadata, comments, editor-specific attributes, and huge path noise.
- Keep filenames lowercase with underscores or the existing local convention.

## Implementation

1. Create or edit the SVG in the correct resource directory.
2. Update QML resource registration if the project uses one.
3. Update the QML component or style file that references the icon.
4. For stateful icons, verify normal/hover/pressed/disabled colors follow existing QML patterns.
5. Put working iterations under `.ai/icon-<name>/` only when useful.

## Verification

Use the narrowest relevant check:

- inspect the SVG directly
- run a client build when resource registration or QML references changed:

```powershell
cmake --preset msvc2022-client-verify
cmake --build --preset msvc2022-client-verify
```

For visual work, run or screenshot the relevant QML surface if a local path/script exists. Do not invent a new renderer unless needed.

## Completion

Report:

- final asset path
- files updated to register/use it
- build or visual check performed
- any manual visual verification still needed
