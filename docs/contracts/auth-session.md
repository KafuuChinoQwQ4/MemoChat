# 认证与会话契约

## 登录链路

客户端通过 HTTP `/user_login` 登录。成功响应包含：

- `uid`
- `token`
- `login_ticket`
- `ticket_expire_ms`
- `refresh_token`
- `protocol_version`
- `user_profile`
- `chat_endpoints`

桌面客户端和 Web 前端都需要保存 HTTP token，并使用 `login_ticket` 完成聊天长连接登录。浏览器端实现位于 `apps/web/src/app/bootstrap/postLoginBootstrap.ts`，QML 客户端对应会话和连接逻辑位于 `apps/client/desktop/MemoChat-qml/app/session`、`app/connection` 和相关 feature 绑定。

## token 与 login_ticket

- `token` 用于 HTTP API 的 `Authorization: Bearer <token>`。
- `login_ticket` 用于 ChatServer 长连接登录，避免直接在长连接握手里复用完整 HTTP 登录凭据。
- Refresh token verifier 和 login ticket 签名密钥必须按 `apps/server/core/docs/secret-management.md` 注入。

## 连接状态

聊天长连接建立后，服务端维护连接 session、心跳和消息投递状态。客户端需要区分：

- HTTP 登录态。
- 聊天传输连接态。
- 关系/会话 bootstrap 是否完成。

不要把传统浏览器 cookie session 概念套到 MemoChat；当前项目使用显式 token、login_ticket 和长连接 session 对象。
