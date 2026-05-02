# Plan

## Task

Fix AI page first-activation message loading, post-answer input focus recovery, and low-contrast input placeholder.

## Implementation

1. In `AgentController::switchSession`, when the clicked session is already current, call `loadHistory(sessionId)` instead of returning silently.
2. In `AgentComposerBar.qml`, add `focusComposerSoon()` and call it:
   - when the component is created,
   - after submit clears the text,
   - when `enabledComposer` changes back to true.
3. Change AI input placeholder color to a darker blue-gray.

## Verification

- Run client CMake configure:
  `cmake --preset msvc2022-client-verify`
- Run client build:
  `cmake --build --preset msvc2022-client-verify`

## Manual UI Check

After launching the client:

1. Enter AI tab, click an existing AI session once. Messages should load immediately.
2. Send a prompt, wait for answer completion, then type again without switching tabs. Caret should appear and text should enter.
3. Confirm input placeholder is readable on the glass background.

## Status

- [x] Context gathered.
- [x] Code updated.
- [x] Client configure passed.
- [x] Client build passed.
- [ ] Manual UI check by user.
