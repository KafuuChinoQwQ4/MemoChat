# cxx_modules/ 目录树

> ChatServer 消息领域的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatMessageInternalGrpcService.cppm` | 内部消息 gRPC 服务错误消息、缺请求/依赖 guard 和 tcp message id 转换算法 module。 |
| `GroupResponse.cppm` | 群组响应格式化使用的权限位判断算法 module。 |
| `MessageDelivery.cppm` | 消息投递通知构造使用的 recipient 选择和文本项有效性算法 module。 |
| `MessageDeliveryService.cppm` | 消息投递服务 recipient 去重 guard、投递/批量 RPC 成功判定、fallback 重试 guard、task 名和 reason 字面量算法 module(仅 gtest 消费)。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
