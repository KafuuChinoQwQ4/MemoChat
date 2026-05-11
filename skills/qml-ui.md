---
description: Implement or review MemoChat QML UI changes, resources, icons, client build verification, and visual/runtime checks.
---

# MemoChat QML UI

Use for MemoChat desktop QML, MemoOps QML, shared client components, icons, resources, layout fixes, and visual behavior.

## Relevant Areas

- `apps/client/desktop/MemoChat-qml`
- `apps/client/desktop/MemoChatShared`
- `apps/client/desktop/CMakeLists.txt`
- `infra/Memo_ops/client/MemoOps-qml`
- `infra/Memo_ops/config/memoops-qml.ini`
- `apps/server/config/memoops-qml.ini`

## Discovery

Search for existing patterns before adding new UI:

```bash
rg -n "<ComponentName>|property |signal |Loader|StackView|ListView|Repeater|Image|Button" apps/client infra/Memo_ops/client
rg -n "qml.qrc|resources|icons|assets" apps/client infra/Memo_ops/client
```

Check whether the UI depends on server endpoints or runtime configs. If yes, include those in the plan.

## Design Rules

- Match existing QML component patterns and spacing.
- Keep text fitting inside controls on narrow windows.
- Use existing icon/resource conventions.
- Avoid adding new global style systems unless the project already has one.
- For stateful controls, handle normal, hover, pressed, disabled, loading, and error states when applicable.
- Do not hardcode service URLs when config already provides them.
- Cross-platform UI changes must be compatibility-first. When fixing Linux/WSLg, macOS, Windows, Wayland/X11, DPI, font, compositor, or graphics-backend differences, preserve the existing behavior of platforms that already work and add the fix through platform-specific QML folders, platform-guarded C++/QML branches, resource aliases, or narrow compatibility components. Do not rewrite a shared component or replace a working platform visual path unless the bug is proven to be shared across those platforms.
- If a shared QML component must be touched for a platform fix, keep its public API and default rendering compatible with the current working platform, then add opt-in properties or platform-specific wrappers for the new behavior. Record the platform boundary in the plan/review and verify at least the changed platform plus any platform whose shared path was touched.
- Prefer additive platform structure: `qml/linux`, `qml/windows`, `qml/macos`, `Qt.platform.os` checks, or `#ifdef Q_OS_*` C++ glue. Keep platform resource registration explicit in `qml.qrc`, and avoid deleting or replacing older platform files while adding a new compatibility path.
- For platform glass/acrylic fixes, keep Windows/macOS behavior on their existing platform path and put WSLg/Linux compatibility in `qml/linux` or platform-guarded code. Do not replace Windows DWM Acrylic with Linux fallbacks.
- On WSLg/Linux, do not use unmasked `ShaderEffectSource`/`MultiEffect` captures that create rectangular backing plates behind rounded glass controls or logos. If system backdrop blur is unavailable, simulate glass with rounded, anti-aliased tint/highlight/border layers only.
- Transparent frameless Linux windows must preserve rounded corner antialiasing: every visible backdrop/control layer needs matching radius, `antialiasing: true`, and no square parent fill behind it. Prefer an alpha buffer plus QML-drawn rounded layers with a small transparent inset; avoid whole-window QML `layer.*` and avoid `QRegion` masks for soft glass edges because both can produce visible rectangular or jagged edges under WSLg.
- For WSLg top-level rounded windows, do not rely on `Rectangle.clip` to clip child pages to a radius; it clips to the rectangular item bounds. Use a Linux-only rounded shell drawn with `QtQuick.Shapes` or an equivalent antialiased path, then inset square page/backdrop content far enough that it starts inside the curve.

## Implementation

- Update QML and resource registration together.
- Keep C++/QML interface property names consistent.
- If adding new backend data, update DTO/API/server response handling at the same time.
- Put throwaway screenshots or notes under `.ai/<project>/<letter>/screenshots` or logs.

## Verification

For Linux client checks, build the client preset. Use `linux-full-gcc16` when the QML change also needs Linux server/runtime integration.

```bash
source /root/.memochat-linux-env
cmake --preset linux-client-gcc16
cmake --build --preset linux-client-gcc16 --parallel 12
```

For runtime UI checks, start the required Linux services first if the screen needs backend data. Use screenshots when possible. If no automated UI runner exists, record the exact manual visual check needed. For explicit Windows client checks, use the legacy `msvc2022-full` and `.bat/.ps1` flow.

## Review Checklist

- no overlapping text at expected window sizes
- no square backing plates behind rounded glass controls, logos, or transparent window corners
- platform-specific compatibility fixes are additive and do not change working platform paths such as Windows Acrylic unless explicitly intended and verified
- shared QML/C++ touched for a platform fix preserves existing defaults and exposes opt-in compatibility behavior
- no missing resource paths
- no broken imports
- no hardcoded local-only path unless it is a documented dev config
- client/server contract remains compatible
- error/loading/empty states are handled where relevant
