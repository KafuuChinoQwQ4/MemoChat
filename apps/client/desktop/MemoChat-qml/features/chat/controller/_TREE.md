# controller/ 目录树

> 聊天 feature 的控制器层：协调对话列表、私聊/群聊会话、收发与已读，并按职责拆成多个实现片段。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatController.cpp` | 早期聊天控制器实现，桥接 ClientGateway 与上层 |
| `ChatController.h` | ChatController 类声明 |
| `ChatDialogRuntimeState.h` | 对话运行时状态结构（好友/群信息等运行态数据） |
| `ChatFeatureController.cpp` | 聊天 feature 主控制器核心实现 |
| `ChatFeatureController.h` | ChatFeatureController 类声明与依赖聚合 |
| `ChatFeatureControllerDialogDecorations.cpp` | 对话列表项的装饰信息（头像/名称等）处理片段 |
| `ChatFeatureControllerDialogDraftCommands.cpp` | 对话草稿相关命令处理片段 |
| `ChatFeatureControllerDialogList.cpp` | 对话列表加载与维护片段 |
| `ChatFeatureControllerDialogMeta.cpp` | 对话元数据（标题/排序等）处理片段 |
| `ChatFeatureControllerDialogReadCommands.cpp` | 对话已读命令处理片段 |
| `ChatFeatureControllerDialogRuntime.cpp` | 对话运行时状态维护片段 |
| `ChatFeatureControllerDialogRuntimeInternal.h` | 对话运行时片段共用的内部声明 |
| `ChatFeatureControllerGroupConversation.cpp` | 群聊会话控制逻辑片段 |
| `ChatFeatureControllerIncoming.cpp` | 入站消息处理片段 |
| `ChatFeatureControllerPendingSend.cpp` | 待发送队列处理片段 |
| `ChatFeatureControllerPrivateConversation.cpp` | 私聊会话控制逻辑片段 |
| `ChatFeatureControllerPrivateHistory.cpp` | 私聊历史拉取处理片段 |
| `ChatFeatureControllerPrivateSend.cpp` | 私聊发送处理片段 |
| `ChatPendingSendQueueState.h` | 待发送队列状态与附件命令结果结构 |
| `ChatReadAckCommand.h` | 已读回执命令与端口结构定义 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
