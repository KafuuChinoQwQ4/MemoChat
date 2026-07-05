# transport/ 目录树

> ChatServer 接入层：统一会话状态、TCP/QUIC/WebSocket/WebTransport 接入模块、共享聊天帧编解码、接入协调器，以及对外的 ChatService gRPC 实现。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | transport 层轻量算法使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CServer.cpp` | TCP 接入服务器实现 |
| `CServer.hpp` | TCP 接入服务器声明 |
| `CSession.cpp` | TCP 兼容会话基类及旧接口适配实现 |
| `CSession.hpp` | TCP 兼容会话基类及旧接口适配声明 |
| `ChatFrameCodec.cpp` | 固定 4 字节聊天帧编解码实现 |
| `ChatFrameCodec.hpp` | 固定 4 字节聊天帧编解码声明 |
| `ChatFrameDispatch.cpp` | transport 入站帧投递到 LogicSystem 的共享实现 |
| `ChatFrameDispatch.hpp` | transport 入站帧投递声明 |
| `ChatIngressCoordinator.cpp` | 接入协调器（TCP/QUIC/WebSocket/WebTransport 启停编排）实现 |
| `ChatIngressCoordinator.hpp` | 接入协调器声明 |
| `ChatSessionCleanupSupport.cpp` | 在线会话异常关闭时清理 UserMgr/Redis 路由状态的共享实现 |
| `ChatSessionCleanupSupport.hpp` | 在线会话异常清理 helper 声明 |
| `ChatSessionState.hpp` | transport-neutral 会话 id、uid、心跳和在线路由刷新状态 |
| `ChatServiceImpl.cpp` | ChatService gRPC 服务实现 |
| `ChatServiceImpl.hpp` | ChatService gRPC 服务声明 |
| `IChatSession.hpp` | transport-neutral 会话接口 |
| `IWebTransportProvider.hpp` | WebTransport 第三方 provider 接入接口 |
| `IWebTransportSession.hpp` | WebTransport provider 可见的会话句柄接口 |
| `LibwebsocketsWebTransportProvider.cpp` | 基于 HTTP/3-capable libwebsockets 的可选 WebTransport provider 实现 |
| `LibwebsocketsWebTransportProvider.hpp` | 基于 libwebsockets 的可选 WebTransport provider 声明 |
| `MsgNode.cpp` | 消息节点（收发缓冲/编解码）实现 |
| `MsgNode.hpp` | 消息节点声明 |
| `QuicChatServer.cpp` | QUIC 接入服务器实现 |
| `QuicChatServer.hpp` | QUIC 接入服务器声明 |
| `QuicSession.cpp` | QUIC 客户端会话实现 |
| `QuicSession.hpp` | QUIC 会话声明 |
| `TcpSession.cpp` | TCP 客户端会话读写循环实现 |
| `TcpSession.hpp` | TCP 客户端会话声明 |
| `UnavailableWebTransportProvider.cpp` | 未配置 HTTP/3 WebTransport 库时的 provider 实现 |
| `UnavailableWebTransportProvider.hpp` | 未配置 HTTP/3 WebTransport 库时的 provider 声明 |
| `WebSocketChatServer.cpp` | WebSocket 接入服务器实现 |
| `WebSocketChatServer.hpp` | WebSocket 接入服务器声明 |
| `WebSocketSession.cpp` | WebSocket 客户端会话读写实现 |
| `WebSocketSession.hpp` | WebSocket 客户端会话声明 |
| `WebTransportChatServer.cpp` | WebTransport provider 驱动接入服务器实现 |
| `WebTransportChatServer.hpp` | WebTransport provider 驱动接入服务器声明 |
| `WebTransportProviderFactory.cpp` | 根据构建开关选择默认 WebTransport provider 的工厂实现 |
| `WebTransportProviderFactory.hpp` | 默认 WebTransport provider 工厂声明 |
| `WebTransportSession.cpp` | WebTransport 聊天帧会话收发与 provider 回调适配实现 |
| `WebTransportSession.hpp` | WebTransport 聊天帧会话声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
