# 离线消息推送失败问题修复

**日期**：2026-03-23
**问题**：客户端登录后，离线消息（对方在用户离线时发送的消息）无法收到

---

## 问题现象

- 客户端登录成功，但登录后对话列表为空
- 对方在用户离线期间发送的消息，登录后未出现在聊天界面
- 服务端日志显示登录流程正常，无报错

---

## 根因分析

本次修复涉及 **客户端** 和 **服务端** 两个层面的问题：

### 1. 客户端：TCP 缓冲区解析 Bug

**文件**：`client/MemoChatShared/tcpmgr.cpp`

服务端登录成功后，会同时推送离线消息（MSG ID `ID_NOTIFY_TEXT_CHAT_MSG_REQ`，即 1002）。由于 `readyRead` 在同一批次中可能收到多个帧：

```
[登录响应 1006][离线消息推送 1002][心跳响应][...]
```

原代码使用 `QByteArray::fromRawData` **零拷贝引用**缓冲区内容，然后在调用 `handleMsg` 之前执行 `memmove` 将缓冲区前移：

```cpp
// 修改前
const QByteArray messageBody = QByteArray::fromRawData(_buffer.constData(), _message_len);
::memmove(_buffer.data(), _buffer.data() + _message_len, _buffer.size() - _message_len);
// handleMsg 执行时，缓冲区已被后续帧覆盖，导致 JSON 解析失败
handleMsg(ReqId(_message_id), _message_len, messageBody);
```

当 `readyRead` 同一批次中有多个帧时，后续帧的 `memmove` 会覆盖当前帧的数据，导致消息体内容被破坏，JSON 解析失败、消息丢失。

### 2. 服务端：离线消息推送逻辑缺失

**文件**：`server/ChatServer/ChatSessionService.cpp`

原版 `HandleLogin` 中没有离线消息推送逻辑。用户重新上线时，服务端从未主动将离线期间积压的消息推送给客户端。

---

## 修复方案

### 1. 客户端：深拷贝替代零拷贝引用

```cpp
// 修改后：在 memmove 之前先复制消息体
const QByteArray messageBody(_buffer.constData(), _message_len);
::memmove(_buffer.data(), _buffer.data() + _message_len, _buffer.size() - _message_len);
_buffer.chop(_message_len);
_b_recv_pending = false;
handleMsg(ReqId(_message_id), _message_len, messageBody);
```

同一修改也应用于 `client/MemoChatShared/QuicChatTransport.cpp`。

### 2. 服务端：新增 `PushOfflineMessages` 离线推送流程

在 `ChatSessionService` 中新增 `PushOfflineMessages` 方法，集成到 `HandleLogin` 的登录成功路径：

```
HandleLogin 成功
    │
    ├── 修复 Redis 在线路由（USERIPPREFIX / USER_SESSION_PREFIX / SERVER_ONLINE_USERS_PREFIX）
    ├── 修复本地 UserMgr 会话映射
    └── PushOfflineMessages（独立线程，异步推送）
            │
            ├── 步骤 1：从 chat_dialog 查出所有好友的 last_read_ts，取最大值作为基准
            │           （确保只推送真正未读的消息）
            │
            ├── 步骤 2：若无未读，直接返回（避免无效查询）
            │
            └── 步骤 3：分批查询 chat_private_msg
                        WHERE to_uid = $uid AND created_at > $latest_read_ts
                              AND deleted_at_ms = 0
                        ORDER BY created_at ASC, msg_id ASC
                        LIMIT 50（每批最多 50 条，最多 10 批）

                        按发送者分组，通过 session->Send(JSON, ID_NOTIFY_TEXT_CHAT_MSG_REQ) 逐条推送
```

**新增数据访问方法**：

- `PostgresDao::GetPrivateDialogRuntime`：从 `chat_dialog` 表查指定对话的 `unread_count` 和 `last_msg_ts`（支持 PostgreSQL 和 MySQL 两套后端）
- `PostgresDao::GetUndeliveredPrivateMessages`：从 `chat_private_msg` 表查 `to_uid` 的未读消息（`created_at > since_read_ts`，排除已删除），返回完整消息字段（msg_id、content、reply_to_server_msg_id、forward_meta_json、edited_at_ms、deleted_at_ms、created_at）

**心跳保活增强**：`HandleHeartbeat` 中增加了会话映射修复逻辑，确保心跳刷新时 Redis 路由状态和 UserMgr 会话映射始终一致，防止因跨服务器查询导致路由失效。

---

## 修改文件清单

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `client/MemoChatShared/tcpmgr.cpp` | 修复 Bug | 消息体深拷贝替代零拷贝引用 |
| `client/MemoChatShared/QuicChatTransport.cpp` | 修复 Bug | 同上，QUIC 传输层 |
| `server/ChatServer/ChatSessionService.cpp` | 新增功能 | 新增 `PushOfflineMessages` 及相关修复 |
| `server/ChatServer/ChatSessionService.h` | 新增声明 | 同上 |
| `server/ChatServer/PostgresDao.cpp` | 新增方法 | `GetUndeliveredPrivateMessages`、`GetPrivateDialogRuntime` |
| `server/ChatServer/PostgresDao.h` | 新增声明 | 同上 |
| `server/ChatServer/PostgresMgr.cpp` | 透传 | 透传上述两个新方法 |
| `server/ChatServer/PostgresMgr.h` | 透传 | 同上 |

---

## 验证方式

1. 客户端 A 登录后，退出（离线）
2. 客户端 B 向 A 发送 1-2 条消息
3. 客户端 A 重新登录
4. 观察客户端 A 的对话列表，确认历史消息已出现

服务端关键日志（离线消息推送）：

```
{"event":"chat.login.offline_push_start","message":"starting offline message push",...}
{"event":"chat.login.offline_push_batch","message":"pushed offline messages batch",...}
{"event":"chat.login.offline_push_done","message":"offline message push completed",...}
{"event":"chat.login.offline_push_skipped","message":"no unread messages",...}
```

---

## 附：离线消息推送参数说明

| 参数 | 值 | 说明 |
|------|----|------|
| `kOfflineBatchSize` | 50 | 每批最多推送 50 条消息 |
| `kMaxOfflineBatches` | 10 | 最多 10 批，单次登录最多推送 500 条离线消息 |
| 基准时间戳 | 所有好友 `last_read_ts` 的最大值 | 避免推送登录前已读的消息 |

推送消息时，排除以下情况：
- 自己发送给自己的消息（`from_uid == to_uid`）
- 已删除消息（`deleted_at_ms > 0`）
- 早于或等于用户最后已读时间戳的消息（`created_at <= latest_read_ts`）
