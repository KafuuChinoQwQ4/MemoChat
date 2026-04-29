Summary: improve delete confirmation UI and immediate AI message visibility.

Approach:
1. Replace the native delete Dialog in ChatLeftPanel with a custom modal Popup using GlassSurface and GlassButton controls.
2. Update AgentConversationPane to keep the message list pinned to the bottom when new messages arrive or streaming content grows.
3. Make AgentMessageModel row updates use indexOfMessage(msgId) directly before changing row data.
4. Verify with diff check and full CMake build.

Status:
- [x] Context gathered
- [x] Plan assessed against existing symbols
- [x] Implement UI popup
- [x] Implement immediate conversation refresh/scroll
- [x] Build verification
- [x] Review diff

Assessed: yes
