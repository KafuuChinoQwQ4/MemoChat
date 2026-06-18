# controller/ 目录树

> 应用主控制器层，AppController 拆分为众多业务切片（会话/群组/私聊/联系人/媒体/分页等状态与绑定）及依赖注入工厂。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppChatDialogBinding.cpp` | 聊天会话绑定切片 |
| `AppChatGroupBinding.cpp` | 群聊绑定切片 |
| `AppChatHistoryBinding.cpp` | 聊天历史绑定切片 |
| `AppChatMediaBinding.cpp` | 聊天媒体绑定切片 |
| `AppChatProjectionBinding.cpp` | 聊天投影绑定切片 |
| `AppChatReadMutationBinding.cpp` | 已读状态变更绑定切片 |
| `AppChatSendBinding.cpp` | 消息发送绑定切片 |
| `AppController.cpp` | 应用主控制器实现 |
| `AppController.h` | 应用主控制器接口声明 |
| `AppControllerBootstrapState.cpp` | 控制器引导状态切片 |
| `AppControllerChatFeatureBinding.cpp` | 聊天功能绑定切片 |
| `AppControllerContactEvents.cpp` | 联系人事件处理切片 |
| `AppControllerContactState.cpp` | 联系人状态切片 |
| `AppControllerDialogListPorts.cpp` | 会话列表端口切片 |
| `AppControllerDialogMetaResponses.cpp` | 会话元信息响应处理切片 |
| `AppControllerDialogModels.cpp` | 会话模型切片 |
| `AppControllerDialogRuntimeState.cpp` | 会话运行时状态切片 |
| `AppControllerDialogSelection.cpp` | 会话选择切片 |
| `AppControllerDialogState.cpp` | 会话状态切片 |
| `AppControllerGroupCommands.cpp` | 群组命令切片 |
| `AppControllerGroupEvents.cpp` | 群组事件切片 |
| `AppControllerGroupHistoryResponses.cpp` | 群组历史响应处理切片 |
| `AppControllerGroupManagementResponses.cpp` | 群组管理响应处理切片 |
| `AppControllerGroupMessageResponses.cpp` | 群组消息响应处理切片 |
| `AppControllerGroupProperties.cpp` | 群组属性切片 |
| `AppControllerGroupResponseErrors.cpp` | 群组响应错误处理切片 |
| `AppControllerGroupResponses.cpp` | 群组响应处理切片 |
| `AppControllerGroupSelection.cpp` | 群组选择切片 |
| `AppControllerIncomingBuffer.cpp` | 入站消息缓冲切片 |
| `AppControllerLoadingState.cpp` | 加载状态切片 |
| `AppControllerMediaUploadQueue.cpp` | 媒体上传队列切片 |
| `AppControllerMessageDispatch.cpp` | 消息分发切片 |
| `AppControllerModelProperties.cpp` | 模型属性切片 |
| `AppControllerModels.cpp` | 模型集合切片 |
| `AppControllerNavigation.cpp` | 导航切片 |
| `AppControllerPagination.cpp` | 分页切片 |
| `AppControllerPendingAttachments.cpp` | 待发送附件切片 |
| `AppControllerPrivateEvents.cpp` | 私聊事件切片 |
| `AppControllerPrivateHistory.cpp` | 私聊历史切片 |
| `AppControllerPrivateMessageResponses.cpp` | 私聊消息响应处理切片 |
| `AppControllerPrivateSelection.cpp` | 私聊选择切片 |
| `AppControllerProfileState.cpp` | 个人资料状态切片 |
| `AppControllerRelationBootstrap.cpp` | 关系引导切片 |
| `AppControllerRuntimeState.h` | 控制器运行时状态定义 |
| `AppControllerStatusState.cpp` | 状态信息切片 |
| `AppControllerUiProperties.cpp` | UI 属性切片 |
| `AppControllerUserProperties.cpp` | 用户属性切片 |
| `AppControllerUserState.h` | 当前用户状态定义 |
| `ChatDialogSelectionPortFactory.cpp` | 会话选择端口工厂实现 |
| `ChatDialogSelectionPortFactory.h` | 会话选择端口工厂声明 |
| `ChatEventDependenciesFactory.cpp` | 聊天事件依赖工厂实现 |
| `ChatEventDependenciesFactory.h` | 聊天事件依赖工厂声明 |
| `ContactEventDependenciesFactory.cpp` | 联系人事件依赖工厂实现 |
| `ContactEventDependenciesFactory.h` | 联系人事件依赖工厂声明 |
| `GroupManagementEffectPortFactory.cpp` | 群组管理副作用端口工厂实现 |
| `GroupManagementEffectPortFactory.h` | 群组管理副作用端口工厂声明 |
| `IncomingMessageRouterFactory.cpp` | 入站消息路由工厂实现 |
| `IncomingMessageRouterFactory.h` | 入站消息路由工厂声明 |
| `PrivateHistoryDependenciesFactory.cpp` | 私聊历史依赖工厂实现 |
| `PrivateHistoryDependenciesFactory.h` | 私聊历史依赖工厂声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
