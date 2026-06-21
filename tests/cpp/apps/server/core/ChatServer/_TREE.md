# ChatServer/ 目录树

> 测试 ChatServer 服务，覆盖消息/关系的内部 gRPC 服务、客户端、适配器、工厂及跨节点冒烟。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `CSessionLifetimeTest.cpp` | 验证 CSession 读循环协程化后对端断连即析构、协程帧不泄漏 session |
| `CServerLifetimeTest.cpp` | 验证 CServer accept/timer 循环协程化后 StopTimer 取消 TimerLoop、CServer 析构不泄漏 |
| `ChatEnvelopeCodecTest.cpp` | 验证 Chat 内部任务/事件 envelope 的 typed JSON 编解码与字段清单 |
| `ChatGroupCommandDtosTest.cpp` | 验证 Chat 群组简单命令 request DTO 的兼容字段提取和字段清单 |
| `ChatHistoryCommandDtosTest.cpp` | 验证 Chat 历史消息 request DTO 的游标字段提取和字段清单 |
| `ChatHistoryOutputDtosTest.cpp` | 验证 Chat 历史消息/离线推送输出 DTO 的 JSON shape、动态字段兼容和字段清单 |
| `ChatMessageCommandDtosTest.cpp` | 验证 Chat 私聊/群聊消息命令与稳定通知 DTO 的兼容字段提取、输出 JSON shape 和字段清单 |
| `ChatMessageInternalGrpcServiceTest.cpp` | 验证消息内部 gRPC 服务实现 |
| `ChatRelationCommandDtosTest.cpp` | 验证 Chat 关系命令 request/response、通知/事件 DTO 的兼容字段提取、JSON shape 和字段清单 |
| `ChatRelationGroupDtosTest.cpp` | 验证 Chat 关系/群列表稳定输出 DTO 的 JSON shape 和字段清单 |
| `ChatRelationInternalGrpcServiceTest.cpp` | 验证关系内部 gRPC 服务实现 |
| `ChatUserProfileDtoTest.cpp` | 验证 Chat 用户 profile/cache DTO 的 Glaze 编解码、响应投影和字段清单 |
| `MessageDeliveryPayloadTest.cpp` | 验证消息投递负载结构 |
| `MessageDeliveryTaskPayloadTest.cpp` | 验证消息投递重试任务载荷 DTO 编解码 |
| `MessageGrpcClientTest.cpp` | 验证消息 gRPC 客户端调用 |
| `MessageGrpcServiceAdapterTest.cpp` | 验证消息 gRPC 服务适配器 |
| `MessageRemoteSmokeTest.cpp` | 跨节点消息服务的远端冒烟测试 |
| `MessageServiceFactoryTest.cpp` | 验证消息服务工厂的实例装配 |
| `RelationGrpcClientTest.cpp` | 验证关系 gRPC 客户端调用 |
| `RelationQueryGrpcClientTest.cpp` | 验证关系查询 gRPC 客户端调用 |
| `RelationQueryRemoteSmokeTest.cpp` | 跨节点关系查询的远端冒烟测试 |
| `RelationQueryServiceFactoryTest.cpp` | 验证关系查询服务工厂装配 |
| `RelationServiceFactoryTest.cpp` | 验证关系服务工厂装配 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
