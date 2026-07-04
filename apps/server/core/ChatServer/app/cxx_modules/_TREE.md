# cxx_modules/ 目录树

> ChatServer app 层共享运行时的项目自有 C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatDeliveryWorkerRuntime.cppm` | 投递 worker 入口参数、默认值和运行状态文案的轻量算法 module |
| `ChatMessageServiceRuntime.cppm` | 消息服务入口参数、默认值和禁用事件发布文案的轻量算法 module |
| `ChatRelationQueryServiceRuntime.cppm` | 关系查询服务入口参数、默认值和 logger 名称的轻量算法 module |
| `ChatRelationServiceWorkerRuntime.cppm` | 关系服务 worker 入口参数、默认值和 bus fallback 文案的轻量算法 module |
| `ChatRuntime.cppm` | 运行时角色、开关和任务重试配置归一化算法 module |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
