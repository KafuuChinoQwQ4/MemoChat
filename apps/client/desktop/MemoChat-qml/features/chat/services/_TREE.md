# services/ 目录树

> 聊天服务层：私聊/群聊的发送、接收、历史、已读、对话列表构建、消息编解码与协议路由等。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatDialogListResponseService.cpp` | 对话列表响应的解析与处理实现 |
| `ChatDialogListResponseService.h` | 对话列表响应服务声明 |
| `ChatDialogSelectionService.cpp` | 对话选择/切换逻辑实现 |
| `ChatDialogSelectionService.h` | 对话选择服务声明 |
| `ChatDispatchService.cpp` | 出站聊天包派发实现 |
| `ChatDispatchService.h` | 出站包结构 OutgoingChatPacket 与派发服务声明 |
| `ChatProtocolRouter.cpp` | 聊天协议消息路由分发实现 |
| `ChatProtocolRouter.h` | 协议路由 ChatProtocolRouter 与路由枚举声明 |
| `ConversationSyncService.cpp` | 会话与消息模型同步实现 |
| `ConversationSyncService.h` | 会话同步服务 ConversationSyncService 声明 |
| `DialogListEntryBuilder.cpp` | 对话列表条目构建实现 |
| `DialogListEntryBuilder.h` | 列表条目构建器 DialogListEntryBuilder 声明 |
| `DialogListService.cpp` | 对话列表整体维护服务实现 |
| `DialogListService.h` | 对话列表服务 DialogListService 声明 |
| `DialogListTypes.h` | 对话列表相关公共类型（DialogEntrySeed 等） |
| `GroupConversationAckService.cpp` | 群聊已读/回执服务实现片段 |
| `GroupConversationHistoryService.cpp` | 群聊历史拉取服务实现片段 |
| `GroupConversationIncomingService.cpp` | 群聊入站消息服务实现片段 |
| `GroupConversationMutationService.cpp` | 群聊消息变更服务实现片段 |
| `GroupConversationService.cpp` | 群聊会话服务核心实现 |
| `GroupConversationService.h` | 群聊会话服务 GroupConversationService 声明 |
| `GroupConversationServiceInternal.h` | 群聊会话服务各分片共用内部声明 |
| `IncomingMessageRouter.cpp` | 入站消息按类型路由到对应处理实现 |
| `IncomingMessageRouter.h` | 入站消息路由 IncomingMessageRouter 与就绪态结构声明 |
| `MessageContentCodec.cpp` | 消息内容编解码实现 |
| `MessageContentCodec.h` | 解码结果结构与内容编解码声明 |
| `MessageMutationCommandService.cpp` | 消息变更命令（撤回/编辑等）执行实现 |
| `MessageMutationCommandService.h` | 变更命令服务与命令/目标结构声明 |
| `MessagePayloadService.cpp` | 消息载荷字段组装实现 |
| `MessagePayloadService.h` | 消息更新字段结构与载荷服务声明 |
| `PrivateChatEventService.cpp` | 私聊事件处理服务核心实现 |
| `PrivateChatEventService.h` | 私聊事件服务 PrivateChatEventService 声明 |
| `PrivateChatEventServiceInternal.h` | 私聊事件服务内部声明 |
| `PrivateChatHistoryRequestService.cpp` | 私聊历史请求构建/发起实现 |
| `PrivateChatHistoryRequestService.h` | 私聊历史请求服务 PrivateChatHistoryRequestService 声明 |
| `PrivateChatHistoryService.cpp` | 私聊历史结果处理与入模型实现 |
| `PrivateChatHistoryService.h` | 私聊历史服务 PrivateChatHistoryService 声明 |
| `PrivateChatReadStatusService.cpp` | 私聊已读状态处理实现 |
| `PrivateChatResponseService.cpp` | 私聊响应解析处理实现 |
| `PrivateChatSendService.cpp` | 私聊消息发送实现 |
| `PrivateChatSendService.h` | 私聊发送服务 PrivateChatSendService 声明 |
| `UploadedAttachmentDispatchService.cpp` | 上传完成附件的派发发送实现 |
| `UploadedAttachmentDispatchService.h` | 附件派发服务与目标结构声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
