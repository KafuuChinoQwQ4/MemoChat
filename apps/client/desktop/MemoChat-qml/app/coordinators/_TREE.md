# coordinators/ 目录树

> 跨模块协调器层，封装通话、媒体附件、注册倒计时等需要跨控制器与外部服务协同的运行流程。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppCoordinators.h` | 协调器共享端口结构定义（注册倒计时、会话鉴权等） |
| `CallCoordinator.cpp` | 通话协调器实现 |
| `CallCoordinatorCommands.cpp` | 通话命令处理切片 |
| `CallCoordinatorEvents.cpp` | 通话事件处理切片 |
| `CallCoordinatorHttp.cpp` | 通话相关 HTTP 交互切片 |
| `CallCoordinatorLivekit.cpp` | 通话 LiveKit 集成切片 |
| `CallCoordinatorPayloadPolicy.cpp` | 通话负载策略实现 |
| `CallCoordinatorPayloadPolicy.h` | 通话负载策略声明 |
| `MediaCoordinator.cpp` | 媒体协调器实现 |
| `MediaPendingAttachmentRunner.cpp` | 待发送媒体附件处理器实现 |
| `MediaPendingAttachmentRunner.h` | 待发送媒体附件处理器声明 |
| `RegisterCountdownController.cpp` | 注册倒计时控制器实现 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
