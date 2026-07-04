# cxx_modules/ 目录树

> ChatServer 关系领域的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatRelationInternalGrpcService.cppm` | 内部关系 gRPC 服务错误消息、uid/request/依赖 guard 和 tcp message id 转换算法 module。 |
| `ChatRelationService.cppm` | 关系服务命令 guard、dialog 类型、事件/task 字面量和 mute 归一化算法 module。 |
| `ChatRelationSessionAdapter.cppm` | 关系会话适配器命令/会话 guard 与转发面(search/apply/delete/dialog)计数契约算法 module(仅 gtest 消费)。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
