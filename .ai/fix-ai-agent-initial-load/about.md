# AI assistant initial load and login glass refresh

## Request
Fix the AI assistant page so conversations/sessions load on first entry instead of only after switching away and back. Also brighten the login page frosted glass so it is less gray and closer to the chat UI's white glass palette.

## Relevant files
- apps/client/desktop/MemoChat-qml/AgentController.{h,cpp}: AI sessions/models/history controller.
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml: async Loader creates AgentPane.
- apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml: AI session sidebar.
- apps/client/desktop/MemoChat-qml/qml/LoginPage.qml and components: login glass colors.

## Dependencies
Client-only QML/C++ change. AI session/model APIs still go through existing Gate/AIServer Docker-backed services at runtime; no port/config/schema changes.
