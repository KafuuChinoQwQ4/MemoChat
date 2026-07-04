# connection/ 目录树

> 聊天连接协调层，依据连接快照与策略管理聊天通道的建立、重连与状态切换。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppChatConnectionCoordinator.cpp` | 聊天连接协调器实现 |
| `AppChatConnectionCoordinator.h` | 聊天连接协调器接口与连接快照结构声明 |
| `AppChatConnectionPolicy.cpp` | 聊天连接策略（超时、备用拨号等）实现 |
| `AppChatConnectionPolicy.h` | 聊天连接策略声明 |
| `AppControllerChatBootstrap.cpp` | 控制器聊天连接引导逻辑 |
| `AppControllerConnectionState.h` | 控制器连接状态定义，包含登录后内存态 token/ticket/refresh token |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
