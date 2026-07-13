# ChatServer/ 目录树

> 分层聊天微服务集群，按职责拆为接入（ChatServer）、消息（ChatMessageService）、关系（ChatRelationService/QueryService）、投递（ChatDeliveryWorker）等独立进程；TCP/QUIC 接入 + 内部 gRPC + Kafka/Redis/RabbitMQ 异步总线，Postgres/Mongo/Redis 持久化。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | 各微服务进程的 main 入口与运行时装配 |
| [`clients/`](clients/_TREE.md) | 内部 gRPC 客户端与服务适配器 |
| [`config/`](config/_TREE.md) | 各服务的配置管理类 |
| [`domain/`](domain/_TREE.md) | 消息/关系/会话/投递等领域逻辑与端口抽象 |
| [`infrastructure/`](infrastructure/_TREE.md) | IO 线程池、单例与常量等基础设施 |
| [`messaging/`](messaging/_TREE.md) | 异步事件/任务总线（Kafka/Redis/RabbitMQ） |
| [`persistence/`](persistence/_TREE.md) | Postgres/Mongo/Redis 持久化与仓储 |
| [`transport/`](transport/_TREE.md) | TCP/QUIC 接入层与会话管理 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | ChatServer 构建脚本 |
| `ChatServer.sln` | Visual Studio 解决方案文件 |
| `ChatServer.vcxproj` | VS 工程文件 |
| `ChatServer.vcxproj.filters` | VS 工程筛选器 |
| `PropertySheet.props` | VS 属性表 |
| `chatdeliveryworker1.ini` | 投递 worker 节点配置 |
| `chatmessageservice1.ini` | 消息服务节点配置 |
| `chatrelationquery1.ini` | 关系查询服务节点配置 |
| `chatrelationservice1.ini` | 关系服务节点配置 |
| `chatserver1.ini` | 接入节点 1 配置 |
| `chatserver2.ini` | 接入节点 2 配置 |
| `config.ini` | 通用/默认配置 |
| `start.bat` | Windows 启动脚本 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
