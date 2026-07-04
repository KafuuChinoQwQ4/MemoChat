# network/ 目录树

> 客户端网络层：HTTP/TCP/QUIC 传输实现、聊天帧编解码与按业务分组的消息分发。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatFrameCodec.cpp` | 聊天帧编解码实现 |
| `ChatFrameCodec.h` | 聊天帧编解码声明 |
| `ChatMessageDispatcher.cpp` | 聊天消息分发器核心实现 |
| `ChatMessageDispatcher.h` | 聊天消息分发器声明 |
| `ChatMessageDispatcherAuth.cpp` | 认证类消息分发处理 |
| `ChatMessageDispatcherContacts.cpp` | 联系人类消息分发处理 |
| `ChatMessageDispatcherGroup.cpp` | 群组类消息分发处理 |
| `ChatMessageDispatcherGroupHistory.cpp` | 群组历史消息分发实现 |
| `ChatMessageDispatcherGroupHistory.h` | 群组历史消息分发声明 |
| `ChatMessageDispatcherGroupPayload.cpp` | 群组消息载荷处理实现 |
| `ChatMessageDispatcherGroupPayload.h` | 群组消息载荷处理声明 |
| `ChatMessageDispatcherPrivate.cpp` | 私聊类消息分发处理 |
| `ChatMessageDispatcherSystem.cpp` | 系统类消息分发处理 |
| `HttpMgrRequestUtils.cpp` | HTTP 请求工具实现 |
| `HttpMgrRequestUtils.h` | HTTP 请求工具声明 |
| `IChatTransport.h` | 聊天传输抽象接口 |
| `QuicChatTransport.cpp` | QUIC 聊天传输实现 |
| `QuicChatTransport.h` | QUIC 聊天传输声明 |
| `QuicChatTransportMsquic.cpp` | 基于 MsQuic 的 QUIC 传输实现 |
| `TelemetryUtils.cpp` | 遥测/埋点工具实现，包含 HTTP span URL 脱敏 |
| `TelemetryUtils.h` | 遥测/埋点工具声明，提供 HTTP span URL 脱敏接口 |
| `httpmgr.cpp` | HTTP 管理器实现（请求发起与回调） |
| `httpmgr.h` | HTTP 管理器声明 |
| `tcpmgr.cpp` | TCP 长连接管理器实现 |
| `tcpmgr.h` | TCP 长连接管理器声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
