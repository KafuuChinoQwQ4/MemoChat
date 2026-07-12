# ChatServer/ 目录树

> 测试 ChatServer 服务，覆盖消息/关系的内部 gRPC 服务、客户端、适配器、工厂及跨节点冒烟。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `CSessionLifetimeTest.cpp` | 验证 CSession 异步读回调在对端断连后释放会话、不会泄漏 session |
| `CServerLifetimeTest.cpp` | 验证 CServer accept/timer 回调在 StopTimer 后取消并释放服务器 |
| `AsyncEventDispatcherAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 异步事件分发器算法 module 的桥接 consumer |
| `AsyncEventDispatcherAlgorithmsTest.cpp` | 验证异步事件分发器发布、轮询、payload guard、刷新和日志字面量算法 |
| `ChatEnvelopeCodecTest.cpp` | 验证 Chat 内部任务/事件 envelope 的 typed JSON 编解码、分区键和重试边界 |
| `ChatFrameCodecTest.cpp` | 验证 ChatServer transport 固定帧编解码、半包和合包处理 |
| `ChatConfigTest.cpp` | 验证 ChatServer 配置目标通过 C++ module 算法处理 feature flag、整数边界和服务后端归一化 |
| `ChatGroupCommandDtosTest.cpp` | 验证 Chat 群组简单命令 request DTO 的兼容字段提取和字段清单 |
| `ChatHandlerRegistrarAlgorithmsConsumer.cpp` | 测试侧导入 ChatHandlerRegistrars 编排注册算法 module 的桥接 consumer |
| `ChatHandlerRegistrarAlgorithmsTest.cpp` | 验证 ChatHandlerRegistrars 各处理器数量、总数和空 dispatcher 判定算法 |
| `ChatHistoryCommandDtosTest.cpp` | 验证 Chat 历史消息 request DTO 的游标字段提取和字段清单 |
| `ChatHistoryOutputDtosTest.cpp` | 验证 Chat 历史消息/离线推送输出 DTO 的 JSON shape、动态字段兼容和字段清单 |
| `ChatGrpcClientAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 节点间聊天 gRPC 客户端算法 module 的桥接 consumer |
| `ChatGrpcClientAlgorithmsTest.cpp` | 验证节点间聊天 gRPC 客户端自节点跳过、连接池默认值和远程调用 guard 算法 |
| `ChatMessageCommandDtosTest.cpp` | 验证 Chat 私聊/群聊消息命令与稳定通知 DTO 的兼容字段提取、输出 JSON shape 和字段清单 |
| `ChatMessageInternalGrpcServiceAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 内部消息 gRPC 服务算法 module 的桥接 consumer |
| `ChatMessageInternalGrpcServiceAlgorithmsTest.cpp` | 验证内部消息 gRPC 服务错误消息、缺请求/依赖 guard 和 tcp message id 转换算法 |
| `ChatMessageInternalGrpcServiceTest.cpp` | 验证消息内部 gRPC 服务实现 |
| `ChatMessageServiceRuntimeAlgorithmsConsumer.cpp` | 测试侧导入 ChatMessageService 入口运行时算法 module 的桥接 consumer |
| `ChatMessageServiceRuntimeAlgorithmsTest.cpp` | 验证消息服务入口启动参数、默认值和禁用事件发布文案算法 |
| `ChatRuntimeAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 运行时算法 module 的桥接 consumer |
| `ChatRuntimeAlgorithmsTest.cpp` | 验证 ChatRuntime 运行时角色、开关和重试下限算法 |
| `ChatRuntimeCompositionAlgorithmsConsumer.cpp` | 测试侧导入 ChatRuntimeComposition 编排算法 module 的桥接 consumer |
| `ChatRuntimeCompositionAlgorithmsTest.cpp` | 验证 ChatRuntimeComposition 任务总线/异步事件总线 backend 选择与 fallback 判定算法 |
| `ChatSessionRepositoryAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 会话仓储算法 module 的桥接 consumer |
| `ChatSessionRepositoryAlgorithmsTest.cpp` | 验证 Chat 会话仓储重复登录锁 uid 和释放 guard 决策 |
| `DistLockAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer Redis 分布式锁算法 module 的桥接 consumer |
| `DistLockAlgorithmsTest.cpp` | 验证 Redis 分布式锁 acquire/release reply 判定算法 |
| `ChatRelationCommandDtosTest.cpp` | 验证 Chat 关系命令 request/response、通知/事件 DTO 的兼容字段提取、JSON shape 和字段清单 |
| `ChatRelationGroupDtosTest.cpp` | 验证 Chat 关系/群列表稳定输出 DTO 的 JSON shape 和字段清单 |
| `ChatRelationInternalGrpcServiceAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 内部关系 gRPC 服务算法 module 的桥接 consumer |
| `ChatRelationInternalGrpcServiceAlgorithmsTest.cpp` | 验证内部关系 gRPC 服务错误消息、uid/request/依赖 guard 和 tcp message id 转换算法 |
| `ChatRelationInternalGrpcServiceTest.cpp` | 验证关系内部 gRPC 服务实现 |
| `ChatRelationQueryServiceRuntimeAlgorithmsConsumer.cpp` | 测试侧导入 ChatRelationQueryService 入口运行时算法 module 的桥接 consumer |
| `ChatRelationQueryServiceRuntimeAlgorithmsTest.cpp` | 验证关系查询服务入口启动参数、默认值和 logger 名称算法 |
| `ChatRelationServiceWorkerRuntimeAlgorithmsConsumer.cpp` | 测试侧导入 ChatRelationServiceWorker 入口运行时算法 module 的桥接 consumer |
| `ChatRelationServiceWorkerRuntimeAlgorithmsTest.cpp` | 验证关系服务 worker 入口启动参数、默认值和 bus fallback 文案算法 |
| `ChatRelationServiceAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 关系服务算法 module 的桥接 consumer |
| `ChatRelationServiceAlgorithmsTest.cpp` | 验证关系服务搜索、好友、dialog guard、事件/task 字面量和 mute 归一化算法 |
| `ChatUserProfileDtoTest.cpp` | 验证 Chat 用户 profile/cache DTO 的 Glaze 编解码、响应投影和字段清单 |
| `ChatUserSupportAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 用户支撑算法 module 的桥接 consumer |
| `ChatUserSupportAlgorithmsTest.cpp` | 验证用户支撑工具数字、缓存、缺用户 guard 和好友申请分页算法 |
| `ChatDeliveryWorkerRuntimeAlgorithmsConsumer.cpp` | 测试侧导入 ChatDeliveryWorker 入口运行时算法 module 的桥接 consumer |
| `ChatDeliveryWorkerRuntimeAlgorithmsTest.cpp` | 验证投递 worker 入口启动参数、默认值和运行状态文案算法 |
| `DeliveryRuntimeAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 投递运行时算法 module 的桥接 consumer |
| `DeliveryRuntimeAlgorithmsTest.cpp` | 验证投递运行时启动、停止、join 和 loop guard 算法 |
| `InlineTaskBusAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer Inline 任务总线算法 module 的桥接 consumer |
| `InlineTaskBusAlgorithmsTest.cpp` | 验证 Inline 任务总线路由、可消费时间、重试丢弃和延迟算法 |
| `KafkaAsyncEventBusAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer Kafka 事件总线算法 module 的桥接 consumer |
| `KafkaAsyncEventBusAlgorithmsTest.cpp` | 验证 Kafka 事件总线发布、消费、DLQ 和错误字面量算法 |
| `MessageDeliveryAlgorithmsTest.cpp` | 验证消息投递 module-backed recipient 选择和文本项过滤算法 |
| `MessageDeliveryPayloadTest.cpp` | 验证消息投递负载结构 |
| `MessageDeliveryTaskPayloadTest.cpp` | 验证消息投递重试任务载荷 DTO 编解码 |
| `MessageRepositoryAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 消息仓储算法 module 的桥接 consumer |
| `MessageRepositoryAlgorithmsTest.cpp` | 验证消息仓储 Postgres/Mongo 双写和读回退决策 |
| `MessagingConfigAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer messaging 配置算法 module 的桥接 consumer |
| `MessagingConfigAlgorithmsTest.cpp` | 验证 ChatServer messaging 配置 module 的整数、布尔和下限归一化算法 |
| `MongoDaoAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer MongoDao 算法 module 的桥接 consumer |
| `MongoDaoAlgorithmsTest.cpp` | 验证 MongoDao 启用、分页、游标、编辑和撤回 guard 决策 |
| `MongoMgrAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer MongoMgr 转发表面算法 module 的桥接 consumer |
| `MongoMgrAlgorithmsTest.cpp` | 验证 MongoMgr DAO 转发表面数量契约算法 |
| `MsgNodeAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 消息节点帧头算法 module 的桥接 consumer |
| `MsgNodeAlgorithmsTest.cpp` | 验证消息节点帧头长度和字段偏移算法 |
| `MessageGrpcClientAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 消息 gRPC 客户端算法 module 的桥接 consumer |
| `MessageGrpcClientAlgorithmsTest.cpp` | 验证消息 gRPC 客户端方法名、远程错误字段和 gRPC/payload guard 决策 |
| `MessageGrpcClientTest.cpp` | 验证消息 gRPC 客户端调用 |
| `MessageGrpcServiceAdapterTest.cpp` | 验证消息 gRPC 服务适配器 |
| `MessageRemoteSmokeTest.cpp` | 跨节点消息服务的远端冒烟测试 |
| `MessageServiceFactoryTest.cpp` | 验证消息服务工厂的实例装配 |
| `OnlineRouteStoreAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer Redis 在线路由存储算法 module 的桥接 consumer |
| `OnlineRouteStoreAlgorithmsTest.cpp` | 验证 Redis 在线路由 uid、server-name、session 和 online-set guard 决策 |
| `OnlineRouteResolverTest.cpp` | 验证在线路由解析四分支纯决策 |
| `OutboxAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer outbox retry 算法 module 的桥接 consumer |
| `OutboxAlgorithmsTest.cpp` | 验证 Chat outbox retry 递增、backoff、终止和 repair 调度决策 |
| `PostgresDaoAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresDao 连接配置算法 module 的桥接 consumer |
| `PostgresDaoAlgorithmsTest.cpp` | 验证 PostgresDao 配置段回退、启用、默认 schema/sslmode 和启动池大小决策 |
| `PostgresDaoDialogsAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresDao dialogs 算法 module 的桥接 consumer |
| `PostgresDaoDialogsAlgorithmsTest.cpp` | 验证会话列表、read-state、dialog meta 和群申请分页 guard 决策 |
| `PostgresDaoGroupMessagesAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresDao 群消息算法 module 的桥接 consumer |
| `PostgresDaoGroupMessagesAlgorithmsTest.cpp` | 验证群消息保存、编辑、撤回、历史分页和查找 guard 决策 |
| `PostgresDaoGroupsAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresDao 群管理算法 module 的桥接 consumer |
| `PostgresDaoGroupsAlgorithmsTest.cpp` | 验证群管理创建、申请、角色、权限和生命周期 guard 决策 |
| `PostgresDaoPrivateMessagesAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresDao 私聊消息算法 module 的桥接 consumer |
| `PostgresDaoPrivateMessagesAlgorithmsTest.cpp` | 验证私聊消息读取、分页、编辑、撤回和 read-state guard 决策 |
| `PostgresDaoUsersAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresDao 用户算法 module 的桥接 consumer |
| `PostgresDaoUsersAlgorithmsTest.cpp` | 验证用户公开 ID 和 uid guard 决策 |
| `PostgresMgrAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer PostgresMgr 初始化算法 module 的桥接 consumer |
| `PostgresMgrAlgorithmsTest.cpp` | 验证 PostgresMgr DAO 初始化、析构 reset 和初始化失败日志字面量决策 |
| `RabbitMqTaskBusAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer RabbitMQ 任务总线算法 module 的桥接 consumer |
| `RabbitMqTaskBusAlgorithmsTest.cpp` | 验证 RabbitMQ 任务总线连接、ack/nack、DLQ、队列后缀和错误字面量算法 |
| `RelationBootstrapCacheAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 关系引导缓存算法 module 的桥接 consumer |
| `RelationBootstrapCacheAlgorithmsTest.cpp` | 验证关系引导缓存 schema、uid 和 TTL 决策算法 |
| `RelationRepositoryAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 关系仓储转发表面算法 module 的桥接 consumer |
| `RelationRepositoryAlgorithmsTest.cpp` | 验证关系仓储 Postgres 转发表面数量契约算法 |
| `RelationGrpcClientAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 关系 gRPC 客户端算法 module 的桥接 consumer |
| `RelationGrpcClientAlgorithmsTest.cpp` | 验证关系 gRPC 客户端方法名、远程错误字段和 gRPC/payload guard 决策 |
| `RelationGrpcServiceAdapterAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 关系 gRPC 服务适配器算法 module 的桥接 consumer |
| `RelationGrpcServiceAdapterAlgorithmsTest.cpp` | 验证关系 gRPC 服务适配器转发面数量和默认超时契约算法 |
| `RelationGrpcClientTest.cpp` | 验证关系 gRPC 客户端调用 |
| `RelationQueryGrpcClientAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 关系查询 gRPC 客户端算法 module 的桥接 consumer |
| `RelationQueryGrpcClientAlgorithmsTest.cpp` | 验证关系查询 gRPC 客户端方法名、远程错误字段和 gRPC/payload guard 决策 |
| `RelationQueryGrpcClientTest.cpp` | 验证关系查询 gRPC 客户端调用 |
| `RelationQueryRemoteSmokeTest.cpp` | 跨节点关系查询的远端冒烟测试 |
| `RelationQueryServiceFactoryTest.cpp` | 验证关系查询服务工厂装配 |
| `RelationServiceFactoryTest.cpp` | 验证关系服务工厂装配 |
| `RedisAsyncEventBusAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer Redis 事件总线算法 module 的桥接 consumer |
| `RedisAsyncEventBusAlgorithmsTest.cpp` | 验证 Redis 事件总线序列化、重入队 guard 和错误字面量算法 |
| `RedisMgrAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer RedisMgr 算法 module 的桥接 consumer |
| `RedisMgrAlgorithmsTest.cpp` | 验证 RedisMgr TTL、reply 类型和空锁释放 guard 决策 |
| `SendPathGoldenTest.cpp` | 锁定私聊/群聊发送路径响应根 JSON 的 golden 输出 |
| `TaskDispatcherAlgorithmsConsumer.cpp` | 测试侧导入 ChatServer 任务分发器算法 module 的桥接 consumer |
| `TaskDispatcherAlgorithmsTest.cpp` | 验证任务分发器 task 类型、错误字面量、ack 和 outbox guard 算法 |
| `TaskDispatcherRuntimeTest.cpp` | 验证任务总线显式消费失败时分发循环按停止请求安全退出 |
| `WebSocketIngressTest.cpp` | 验证 WebSocket 接入的真实握手、二进制帧解码、文本帧拒绝和停机关闭 |
| `WebTransportProviderTest.cpp` | 验证 WebTransport provider 接入层的启动、会话注册、发送回调和关闭清理 |
| `WebTransportSessionTest.cpp` | 验证 WebTransport 聊天帧会话的 provider 发送、半包/合包接收和异常关闭 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
