Review:
- Replaced the native AI delete Dialog with a rounded Popup using GlassSurface and two GlassButton actions, matching the rest of the app styling.
- The popup still clears pending delete state on close and emits agentSessionDeleted only after confirming.
- AgentConversationPane now keeps the conversation pinned to the bottom on message insertion, model reset, dataChanged, and content-height changes. It also calls forceLayout before positioning to handle streaming row growth.
- AgentMessageModel now resolves rows by msgId directly before emitting dataChanged, avoiding fragile QVector::indexOf on a mutable entry.

Residual risk:
- I did not run a live desktop click-through in this pass. Build and qmllint checks passed.
