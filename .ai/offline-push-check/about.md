# Offline Push Check

This project verifies and fixes private-message offline push after switching between two logged-in accounts.

Expected behavior: when account A sends private messages while account B is offline, B receives the missed private message notifications on chat login. The server derives unread private messages from persisted `chat_private_msg` rows and per-dialog `chat_private_read_state`, then pushes them to the newly authenticated `CSession`.
