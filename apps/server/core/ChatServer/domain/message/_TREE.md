# message/ 目录树

> 消息领域：私聊与群聊消息服务、内部消息 gRPC、投递载荷与响应格式化，以及通过工厂装配消息命令服务。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatMessageInternalGrpcService.cpp` | 内部消息 gRPC 服务实现 |
| `ChatMessageInternalGrpcService.h` | 内部消息 gRPC 服务声明 |
| `GroupMessageService.cpp` | 群聊消息服务实现 |
| `GroupMessageService.h` | 群聊消息服务声明 |
| `GroupResponseFormatter.cpp` | 群聊响应格式化实现 |
| `GroupResponseFormatter.h` | 群聊响应格式化声明 |
| `MessageDeliveryPayload.cpp` | 消息投递载荷实现 |
| `MessageDeliveryPayload.h` | 投递载荷声明 |
| `MessageDeliveryService.cpp` | 消息投递服务实现 |
| `MessageDeliveryService.h` | 消息投递服务声明 |
| `MessageServiceFactory.cpp` | 消息服务工厂（装配私聊/群聊）实现 |
| `MessageServiceFactory.h` | 消息服务工厂声明 |
| `MessageServiceUtil.h` | 消息服务公共工具 |
| `PrivateMessageService.cpp` | 私聊消息服务实现 |
| `PrivateMessageService.h` | 私聊消息服务声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
