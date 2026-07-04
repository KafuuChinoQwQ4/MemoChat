# cxx_modules/ 目录树

> ChatServer 客户端层的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatGrpcClient.cppm` | 节点间聊天 gRPC 客户端自节点跳过、连接池默认值和远程调用 guard 算法 module。 |
| `MessageGrpcClient.cppm` | 消息 gRPC 客户端方法名、远程错误字段和 payload/status guard 算法 module。 |
| `RelationGrpcClient.cppm` | 关系 gRPC 客户端方法名、远程错误字段和 payload/status guard 算法 module。 |
| `RelationGrpcServiceAdapter.cppm` | 关系 gRPC 服务适配器转发面数量和默认超时契约算法 module。 |
| `RelationQueryGrpcClient.cppm` | 关系查询 gRPC 客户端方法名、远程错误字段和 payload guard 算法 module。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
