Task: add a usable AI session delete feature and add a friends list to the Moments side panel so selecting a friend shows that friend's Moments feed by publish time.

Relevant files:
- apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml
- apps/client/desktop/MemoChat-qml/qml/moments/MomentsFeedPane.qml
- apps/client/desktop/MemoChat-qml/MomentsController.h/.cpp
- apps/server/core/GateServer/MomentsRouteModules.cpp
- apps/server/core/GateServerCore/PostgresDao.h/.cpp
- apps/server/core/GateServerCore/PostgresMgr.h/.cpp
- Additional HTTP moments route variants under GateServerHttp1.1, GateServerHttp2, GateServerHttp3 must keep compiling after signature change.

Findings:
- ChatLeftPanel already had an AI session delete glyph, but the full-row MouseArea was declared after the row content, so it could sit above and steal clicks.
- The Moments side panel component was an empty Item.
- FriendListModel exposes uid, userId, name, nick, icon, desc roles.
- Moments list currently posts /api/moments/list with last_moment_id and limit only. The server query is already sorted by created_at DESC, moment_id DESC and enforces visibility.

Data/services:
- PostgreSQL moments query is touched through GateServerCore PostgresDao.
- Docker ports/config are not changed.

Verification:
- Requested build command: cmake --build --preset msvc2022-full.
