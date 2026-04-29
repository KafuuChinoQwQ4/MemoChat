# Plan

Task summary:
Shrink the AI assistant header feature buttons so the `知识库` button no longer exceeds the header border.

Approach:
- Add local header action sizing constants in `AgentPane.qml`.
- Reduce the row spacing between header controls.
- Apply compact width, height, font size, and corner radius to `模型`, `知识库`, and `轨迹`.
- Disable scale feedback on those compact buttons so hover/pressed states stay within the header boundary.

Files to modify:
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`

Checklist:
- [x] Gather QML context.
- [x] Implement compact header button sizing.
- [x] Verify with a client build or QML syntax/build check.
- [x] Review final diff.

Assessed: yes
