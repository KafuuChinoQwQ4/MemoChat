# message/ 目录树

> 消息领域：私聊与群聊消息服务、内部消息 gRPC、投递载荷与响应格式化，以及通过工厂装配消息命令服务。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 消息领域的 C++ module 接口分组 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatMessageInternalGrpcService.cpp` | 内部消息 gRPC 服务实现，导入轻量算法 module 处理错误消息、缺依赖 guard 和 tcp message id 转换。 |
| `ChatMessageInternalGrpcService.hpp` | 内部消息 gRPC 服务声明 |
| `GroupManagementService.cpp` | 群组管理命令服务实现，处理公告、头像、管理员、禁言、踢人、退出和解散 |
| `GroupManagementService.hpp` | 群组管理命令服务声明 |
| `GroupMembershipService.cpp` | 群组成员生命周期命令服务实现，处理建群、群列表、邀请、申请和审核 |
| `GroupMembershipService.hpp` | 群组成员生命周期命令服务声明 |
| `GroupMessageHistoryService.cpp` | 群聊历史与已读状态服务实现，处理历史查询、读状态更新和已读通知 |
| `GroupMessageHistoryService.hpp` | 群聊历史与已读状态服务声明 |
| `GroupMessageService.cpp` | 群聊消息 façade 与 session handler 实现，委托成员、消息工作流、历史和管理命令 |
| `GroupMessageService.hpp` | 群聊消息 façade 与 session handler 声明 |
| `GroupMessageWorkflow.cpp` | 群聊消息发送、编辑、撤回和转发工作流实现 |
| `GroupMessageWorkflow.hpp` | 群聊消息发送、编辑、撤回和转发工作流声明 |
| `GroupResponseFormatter.cpp` | 群聊响应格式化实现 |
| `GroupResponseFormatter.hpp` | 群聊响应格式化声明 |
| `MessageDeliveryPayload.cpp` | 消息投递载荷实现 |
| `MessageDeliveryPayload.hpp` | 投递载荷声明 |
| `MessageDeliveryService.cpp` | 消息投递服务实现 |
| `MessageDeliveryService.hpp` | 消息投递服务声明 |
| `MessageServiceFactory.cpp` | 消息服务工厂（装配私聊/群聊）实现 |
| `MessageServiceFactory.hpp` | 消息服务工厂声明 |
| `MessageServiceUtil.hpp` | 消息服务公共工具 |
| `PrivateMessageService.cpp` | 私聊消息服务实现 |
| `PrivateMessageService.hpp` | 私聊消息服务声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
