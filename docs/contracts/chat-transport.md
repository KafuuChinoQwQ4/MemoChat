# 聊天传输契约

聊天链路分为 HTTP bootstrap 和长连接传输两部分。

## HTTP bootstrap

1. 客户端调用 `/user_login`。
2. 服务端返回 `token`、`login_ticket`、`protocol_version` 和 `chat_endpoints`。
3. 客户端保存 session，并选择浏览器或桌面可用的聊天 endpoint。
4. 客户端发送聊天登录帧，收到登录确认后进入 ready。
5. 客户端发起关系 bootstrap，拉取好友/申请等初始数据。

Web 实现参考 `apps/web/src/app/bootstrap/postLoginBootstrap.ts`。

## 长连接传输

聊天帧使用 `ReqId + JSON payload` 的协议模型。Web 前端在 `apps/web/src/core/network/transport/` 中支持：

- WebTransport 优先。
- WebSocket fallback。
- Mock transport 用于前端 UI 开发。

Vite 本地开发把 `/ws` 代理到 `localhost:8444`。生产和本地实际 endpoint 以登录响应 `chat_endpoints` 和运行时配置为准。

桌面客户端使用自己的网络层和 ChatServer 长连接入口，业务状态通过 QML feature facade 和 C++ controller/viewmodel 更新。

## 入站分发

收到聊天帧后，传输层只负责解码和派发。业务处理应按 `ReqId` 注册到 dispatcher，再写入 entity store、viewmodel 或对应 service。不要让传输层直接写 feature 业务状态。
