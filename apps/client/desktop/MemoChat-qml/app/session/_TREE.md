# session/ 目录树

> 会话协调层，管理登录鉴权、鉴权定时、连接状态、登出以及登录后聊天入口与关系数据引导。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppSessionCoordinator.cpp` | 会话协调器实现 |
| `AppSessionCoordinatorAuthTimers.cpp` | 会话鉴权定时器切片 |
| `AppSessionCoordinatorConnectionState.cpp` | 会话连接状态切片，维护登录后内存态 token/ticket/refresh token |
| `AppSessionCoordinatorLogout.cpp` | 会话登出切片 |
| `SessionAuthCoordinator.cpp` | 会话鉴权协调器实现 |
| `SessionAuthCoordinatorAuthResponses.cpp` | 鉴权响应处理切片 |
| `SessionAuthCoordinatorCommands.cpp` | 鉴权命令处理切片 |
| `SessionAuthCoordinatorLoginResponse.cpp` | 登录响应处理切片，解析 token/ticket/refresh token 并做脱敏错误日志 |
| `SessionChatEntryCoordinator.cpp` | 登录后聊天入口协调实现 |
| `SessionRelationBootstrap.cpp` | 登录后关系数据引导实现 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
