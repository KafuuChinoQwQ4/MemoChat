# cxx_modules/ 目录树

> ChatServer transport 层的项目自有 C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MsgNode.cppm` | 消息节点帧头长度和字段偏移算法 module |
| `ChatServiceImpl.cppm` | ChatService gRPC 通知的 recipient 会话 guard、delivered 计数、fanout/私聊回复错误码、from_user_id 附加、Postgres 刷新与时间戳回退判定算法 module(仅 gtest 消费)。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
