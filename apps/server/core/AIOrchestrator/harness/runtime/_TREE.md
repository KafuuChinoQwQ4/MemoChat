# runtime/ 目录树

> harness 的运行时基础设施：依赖容器、消息总线与任务服务，将各层装配为可运行整体。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `README.md` | 运行时层说明文档。 |
| `__init__.py` | 包初始化。 |
| `container.py` | 依赖注入容器：组装各 harness 服务。 |
| `message_bus.py` | 消息总线：对接 RabbitMQ/Redpanda 等队列。 |
| `task_service.py` | 异步任务调度服务。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
