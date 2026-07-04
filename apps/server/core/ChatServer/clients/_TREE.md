# clients/ 目录树

> ChatServer 微服务间通信的内部 gRPC 客户端与服务适配器，封装跨节点的聊天/消息/关系/关系查询调用，并通过工厂在本地直连与远程 gRPC 间切换。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | ChatServer 客户端层 C++ module 接口分组。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatGrpcClient.cpp` | 节点间聊天 gRPC 客户端实现，导入轻量算法 module 处理自节点跳过、连接池默认值与远程调用 guard。 |
| `ChatGrpcClient.h` | 聊天 gRPC 客户端声明 |
| `MessageGrpcClient.cpp` | 消息服务 gRPC 客户端实现，导入轻量算法 module 处理方法名、远程错误字段与 payload/status guard。 |
| `MessageGrpcClient.h` | 消息 gRPC 客户端声明 |
| `MessageGrpcServiceAdapter.cpp` | 消息服务的 gRPC 适配器实现 |
| `MessageGrpcServiceAdapter.h` | 消息服务适配器声明 |
| `RelationGrpcClient.cpp` | 关系服务 gRPC 客户端实现，导入轻量算法 module 处理方法名、远程错误字段与 payload/状态 guard。 |
| `RelationGrpcClient.h` | 关系 gRPC 客户端声明 |
| `RelationGrpcServiceAdapter.cpp` | 关系服务的 gRPC 适配器实现，导入轻量算法 module 锁定转发面数量和默认超时契约。 |
| `RelationGrpcServiceAdapter.h` | 关系服务适配器声明 |
| `RelationQueryGrpcClient.cpp` | 关系查询 gRPC 客户端实现，导入轻量算法 module 处理方法名、远程错误字段与 payload guard。 |
| `RelationQueryGrpcClient.h` | 关系查询客户端声明 |
| `RelationQueryServiceFactory.cpp` | 关系查询服务工厂（本地/远程切换）实现 |
| `RelationQueryServiceFactory.h` | 关系查询服务工厂声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
