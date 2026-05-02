# R18 Glass Tone Context

Task date: 2026-05-02

User request: 将 R18 界面的粉色调改成像聊天界面一样的色调和透明毛玻璃效果。

Relevant files:
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml
- apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml

Implementation context:
- ChatShellPage contains the flip deck and shared GlassBackdrop.
- R18ShellPane owns R18 navigation, shelf/search/detail/reader/sources/history UI.
- Previous R18 work already added four-button navigation and direct JM source import staging; this phase only adjusts visual tone/glass styling.

Desired state:
- R18 surface uses MemoChat chat-style translucent white glass panels and blue-gray hover/selected states.
- R18 no longer drives the whole window/backdrop to the old pink acrylic tint.
- Error state may remain reddish because it communicates severity, but the default UI should not read as pink.

Verification plan:
- qmllint R18ShellPane.qml and ChatShellPage.qml.
- git diff --check for touched QML files.
- cmake --build --preset msvc2022-client-verify.
