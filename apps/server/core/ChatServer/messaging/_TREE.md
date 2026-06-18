# messaging/ 目录树

> ChatServer 的异步事件/任务总线抽象及多种实现：Kafka/Redis 事件总线、RabbitMQ/Inline 任务总线，承载消息投递与事件分发的解耦。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AsyncEventDispatcher.cpp` | 异步事件分发器实现 |
| `AsyncEventDispatcher.h` | 异步事件分发器声明 |
| `ChatAsyncEvent.cpp` | 聊天异步事件结构实现 |
| `ChatAsyncEvent.h` | 聊天异步事件声明 |
| `ChatTaskEnvelope.cpp` | 任务信封（序列化载体）实现 |
| `ChatTaskEnvelope.h` | 任务信封声明 |
| `IAsyncEventBus.h` | 异步事件总线接口抽象 |
| `IAsyncTaskBus.h` | 异步任务总线接口抽象 |
| `InlineTaskBus.cpp` | 进程内同步任务总线实现 |
| `InlineTaskBus.h` | Inline 任务总线声明 |
| `KafkaAsyncEventBus.cpp` | Kafka 事件总线实现 |
| `KafkaAsyncEventBus.h` | Kafka 事件总线声明 |
| `KafkaConfig.cpp` | Kafka 配置实现 |
| `KafkaConfig.h` | Kafka 配置声明 |
| `RabbitMqConfig.cpp` | RabbitMQ 配置实现 |
| `RabbitMqConfig.h` | RabbitMQ 配置声明 |
| `RabbitMqTaskBus.cpp` | RabbitMQ 任务总线实现 |
| `RabbitMqTaskBus.h` | RabbitMQ 任务总线声明 |
| `RedisAsyncEventBus.cpp` | Redis 事件总线实现 |
| `RedisAsyncEventBus.h` | Redis 事件总线声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
