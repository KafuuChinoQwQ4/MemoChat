# cxx_modules/ 目录树

> ChatServer messaging 层的项目自有 C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AsyncEventDispatcher.cppm` | 异步事件分发器发布、轮询、payload guard、刷新和日志字面量算法 module |
| `InlineTaskBus.cppm` | Inline 任务总线路由、可消费时间和重试边界算法 module |
| `KafkaAsyncEventBus.cppm` | Kafka 事件总线发布、消费、DLQ 和错误字面量算法 module |
| `MessagingConfig.cppm` | Kafka/RabbitMQ 配置解析和下限归一化算法 module |
| `MessagingEnvelope.cppm` | 任务/事件 envelope 非负归一化和 uid 分区键算法 module |
| `RabbitMqTaskBus.cppm` | RabbitMQ 任务总线连接、ack/nack、DLQ、队列后缀和错误字面量算法 module |
| `RedisAsyncEventBus.cppm` | Redis 事件总线序列化、重入队 guard 和错误字面量算法 module |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
