# delivery/ 目录树

> 投递领域：投递 worker 运行时、任务分发器与投递任务载荷 DTO，负责把消息/关系通知投递任务路由到目标在线会话。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatDeliveryRuntime.cpp` | 投递运行时实现 |
| `ChatDeliveryRuntime.h` | 投递运行时声明 |
| `MessageDeliveryTaskPayload.cpp` | 投递任务载荷 DTO 的 Glaze 编解码实现 |
| `MessageDeliveryTaskPayload.h` | 投递任务载荷 DTO 声明 |
| `TaskDispatcher.cpp` | 投递任务分发器实现 |
| `TaskDispatcher.h` | 任务分发器声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
