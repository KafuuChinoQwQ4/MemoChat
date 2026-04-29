# Context

Task:
Fix the AI assistant header where the `知识库` button can partially exceed the card border. The requested change is to shrink the three adjacent feature buttons so they remain inside the border.

Relevant files:
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/components/GlassButton.qml`

Findings:
- The top header actions in `AgentPane.qml` use fixed `Layout.preferredWidth` values.
- `GlassButton` applies a scale animation on hover and press by default, which can make a button near the edge visually exceed the header boundary even when its layout width fits.
- This is client-only QML layout work. No Docker service, database, or port mapping is involved.

Verification target:
- Run a narrow client/QML build check if feasible.
- Manual visual check: open the AI assistant panel at a narrow window width and confirm `模型`, `知识库`, and `轨迹` remain within the header border in normal, hover, and pressed states.
