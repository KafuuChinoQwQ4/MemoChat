# clients/ 目录树

> ChatServer 微服务间通信的内部 gRPC 客户端与服务适配器，封装跨节点的聊天/消息/关系/关系查询调用，并通过工厂在本地直连与远程 gRPC 间切换。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatGrpcClient.cpp` | 节点间聊天 gRPC 客户端实现 |
| `ChatGrpcClient.h` | 聊天 gRPC 客户端声明 |
| `MessageGrpcClient.cpp` | 消息服务 gRPC 客户端实现 |
| `MessageGrpcClient.h` | 消息 gRPC 客户端声明 |
| `MessageGrpcServiceAdapter.cpp` | 消息服务的 gRPC 适配器实现 |
| `MessageGrpcServiceAdapter.h` | 消息服务适配器声明 |
| `RelationGrpcClient.cpp` | 关系服务 gRPC 客户端实现 |
| `RelationGrpcClient.h` | 关系 gRPC 客户端声明 |
| `RelationGrpcServiceAdapter.cpp` | 关系服务的 gRPC 适配器实现 |
| `RelationGrpcServiceAdapter.h` | 关系服务适配器声明 |
| `RelationQueryGrpcClient.cpp` | 关系查询 gRPC 客户端实现 |
| `RelationQueryGrpcClient.h` | 关系查询客户端声明 |
| `RelationQueryServiceFactory.cpp` | 关系查询服务工厂（本地/远程切换）实现 |
| `RelationQueryServiceFactory.h` | 关系查询服务工厂声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
