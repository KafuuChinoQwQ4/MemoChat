# transport/ 目录树

> ChatServer 接入层：TCP（CServer/CSession）与 QUIC（QuicChatServer/QuicSession）双协议接入、消息节点编解码、接入协调器，以及对外的 ChatService gRPC 实现。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CServer.cpp` | TCP 接入服务器实现 |
| `CServer.h` | TCP 接入服务器声明 |
| `CSession.cpp` | TCP 客户端会话实现 |
| `CSession.h` | TCP 会话声明 |
| `ChatIngressCoordinator.cpp` | 接入协调器（连接接入编排）实现 |
| `ChatIngressCoordinator.h` | 接入协调器声明 |
| `ChatServiceImpl.cpp` | ChatService gRPC 服务实现 |
| `ChatServiceImpl.h` | ChatService gRPC 服务声明 |
| `MsgNode.cpp` | 消息节点（收发缓冲/编解码）实现 |
| `MsgNode.h` | 消息节点声明 |
| `QuicChatServer.cpp` | QUIC 接入服务器实现 |
| `QuicChatServer.h` | QUIC 接入服务器声明 |
| `QuicSession.cpp` | QUIC 客户端会话实现 |
| `QuicSession.h` | QUIC 会话声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
