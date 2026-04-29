# Verification

- `git diff --check -- apps\client\desktop\MemoChat-qml\qml\agent\AgentPane.qml`: passed.
- Full `MemoChatQml` build was attempted earlier but was interrupted by the user before completion, so no build result is recorded for this small follow-up layout change.

Manual visual check needed:
- Open the AI assistant panel at the width shown in the user screenshot.
- Confirm `新会话`, `模型`, `知识库`, and `轨迹` render as a compact 2x2 group inside the header border.
- Hover and press the buttons; scale feedback is disabled for this group, so they should remain within the border.
