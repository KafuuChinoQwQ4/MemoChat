# events/ 目录树

> 事件路由层，把聊天分发服务与 HTTP 响应分别路由到对应的控制器与协调器处理。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppChatDispatcherEventRouter.cpp` | 聊天分发事件路由器实现 |
| `AppChatDispatcherEventRouter.h` | 聊天分发事件路由器声明，聚合各聊天事件服务 |
| `AppChatDispatcherGroupResponseHandlers.h` | 群组响应处理器集合声明 |
| `AppHttpEventRouter.cpp` | HTTP 事件路由器实现 |
| `AppHttpEventRouter.h` | HTTP 事件路由器声明（登录/注册/重置/设置响应） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
