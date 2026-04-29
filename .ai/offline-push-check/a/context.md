# Context

Task: 用户反馈两个账号轮流登录互发消息，离线一方重新登录后收不到对方发来的消息，需要检查离线推送功能。

Relevant files:
- apps/server/core/ChatServer/ChatSessionService.cpp: `PushOfflineMessages` runs after chat login and sends `ID_NOTIFY_TEXT_CHAT_MSG_REQ` to the current session.
- apps/server/core/ChatServer/PrivateMessageService.cpp: saves private messages and only realtime-notifies when recipient route is online.
- apps/server/core/ChatServer/PostgresDao.cpp/.h and PostgresMgr.cpp/.h: private message persistence, read state, dialog runtime, offline/unread query.
- apps/server/migrations/postgresql/business/001_baseline.sql and infra/deploy/local/init/postgresql/001-business.sql: `chat_private_msg`, `chat_dialog`, `chat_private_read_state` schema.

Docker/MCP checks:
- `docker ps --format ...` confirmed memochat-postgres, redis, rabbitmq, redpanda and other infra containers are running with stable published ports.

Finding:
- `PushOfflineMessages` initializes `latest_read_ts` from `chat_dialog.last_msg_ts` for unread dialogs, then calls `GetUndeliveredPrivateMessages(uid, latest_read_ts, ...)`.
- `GetUndeliveredPrivateMessages` queries `WHERE to_uid = ? AND created_at > since_read_ts`; passing the unread dialog's last message timestamp filters out that last unread message and often all unread messages.
- The batch loop does not advance a cursor, so if the query returns exactly the batch size it can push the same rows repeatedly.

Open risks:
- Runtime reproduction depends on current service binaries and local client state. Server build verification is the narrowest useful check after code change.
