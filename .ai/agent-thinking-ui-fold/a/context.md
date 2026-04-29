Task: fix the AI conversation "正在生成回复" status badge overlapping the message area, and change thinking content from always-visible text into a collapsible thinking process panel.

Relevant files:
- apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml
- apps/client/desktop/MemoChat-qml/qml/agent/AgentMessageDelegate.qml

Findings:
- The floating status badge in AgentConversationPane was anchored over the ListView without reserving top space, so it could overlap user messages near the top right.
- AgentMessageDelegate displayed thinkingContent directly in an always-expanded rounded box above the final answer.

Implementation:
- Give the message ListView a larger top margin while loading/streaming so the status badge has reserved space.
- Replace the always-expanded thinking block with a "思考过程" panel.
- The thinking panel body is visible while streaming, collapses automatically when isStreaming becomes false, and can be toggled manually after completion.

Verification target:
- Build the desktop client with CMake.
