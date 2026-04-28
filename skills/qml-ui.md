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

```powershell
rg -n "<ComponentName>|property |signal |Loader|StackView|ListView|Repeater|Image|Button" apps\client infra\Memo_ops\client
rg -n "qml.qrc|resources|icons|assets" apps\client infra\Memo_ops\client
```

Check whether the UI depends on server endpoints or runtime configs. If yes, include those in the plan.

## Design Rules

- Match existing QML component patterns and spacing.
- Keep text fitting inside controls on narrow windows.
- Use existing icon/resource conventions.
- Avoid adding new global style systems unless the project already has one.
- For stateful controls, handle normal, hover, pressed, disabled, loading, and error states when applicable.
- Do not hardcode service URLs when config already provides them.

## Implementation

- Update QML and resource registration together.
- Keep C++/QML interface property names consistent.
- If adding new backend data, update DTO/API/server response handling at the same time.
- Put throwaway screenshots or notes under `.ai/<project>/<letter>/screenshots` or logs.

## Verification

Build client when QML/resources/C++ client glue changed:

```powershell
cmake --preset msvc2022-client-verify
cmake --build --preset msvc2022-client-verify
```

For runtime UI checks, start the required local services first if the screen needs backend data. Use screenshots when possible. If no automated UI runner exists, record the exact manual visual check needed.

## Review Checklist

- no overlapping text at expected window sizes
- no missing resource paths
- no broken imports
- no hardcoded local-only path unless it is a documented dev config
- client/server contract remains compatible
- error/loading/empty states are handled where relevant
