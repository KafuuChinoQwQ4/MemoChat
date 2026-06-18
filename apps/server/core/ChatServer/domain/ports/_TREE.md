# ports/ 目录树

> 领域端口（六边形架构）：定义会话、消息、关系、投递、事件、在线路由、仓储与会话注册表等所有跨层依赖的纯接口，由 persistence/clients/messaging 等提供实现。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `IChatSessionConfig.h` | 会话配置端口 |
| `IChatSessionRepository.h` | 会话仓储端口 |
| `IDeliveryGateway.h` | 投递网关端口 |
| `IDeliveryTaskPublisher.h` | 投递任务发布端口 |
| `IEventPublisher.h` | 事件发布端口 |
| `IGroupMessageService.h` | 群聊消息服务端口 |
| `IMessageCommandService.h` | 消息命令服务端口 |
| `IMessageRepository.h` | 消息仓储端口 |
| `IMessageServiceConfig.h` | 消息服务配置端口 |
| `IOnlineRouteStore.h` | 在线路由存储端口 |
| `IOutboxRepairScheduler.h` | Outbox 修复调度端口 |
| `IPrivateMessageService.h` | 私聊消息服务端口 |
| `IRelationBootstrapCache.h` | 关系引导缓存端口 |
| `IRelationCommandService.h` | 关系命令服务端口 |
| `IRelationQueryService.h` | 关系查询服务端口 |
| `IRelationQueryServiceConfig.h` | 关系查询服务配置端口 |
| `IRelationRepository.h` | 关系仓储端口 |
| `IRelationService.h` | 关系服务端口 |
| `IRelationServiceConfig.h` | 关系服务配置端口 |
| `IRelationSessionService.h` | 关系会话服务端口 |
| `ISessionRegistry.h` | 会话注册表端口 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
