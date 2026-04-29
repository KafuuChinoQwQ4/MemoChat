# Plan

Task summary: Fix private-message offline push so a user receives persisted unread messages after logging back in.

Approach:
- Make the DAO unread query compare each message against that sender's read state for the recipient.
- Use the existing `since_read_ts` parameter as a pagination cursor, not as the unread baseline.
- Simplify login offline push to page unread messages from cursor 0 and advance the cursor after every returned batch.

Files to modify:
- apps/server/core/ChatServer/PostgresDao.cpp
- apps/server/core/ChatServer/ChatSessionService.cpp
- .ai/offline-push-check/a/* task artifacts

Implementation phases:
- [x] Update `GetUndeliveredPrivateMessages` SQL for PostgreSQL and legacy SQL to use `chat_private_read_state` per sender.
- [x] Update `PushOfflineMessages` to remove the incorrect dialog-runtime timestamp prefilter and advance a pagination cursor.
- [x] Verify with server configure/build and, if possible, targeted database/runtime checks.
- [x] Review diff and record verification.

Docker/MCP checks required:
- Keep infrastructure in Docker; do not change compose port mappings.
- Optional Postgres query to inspect current private unread data if needed.

Build/test commands:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`

Assessed: yes
