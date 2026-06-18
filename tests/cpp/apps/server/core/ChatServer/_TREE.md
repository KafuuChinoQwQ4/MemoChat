# ChatServer/ 目录树

> 测试 ChatServer 服务，覆盖消息/关系的内部 gRPC 服务、客户端、适配器、工厂及跨节点冒烟。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `ChatMessageInternalGrpcServiceTest.cpp` | 验证消息内部 gRPC 服务实现 |
| `ChatRelationInternalGrpcServiceTest.cpp` | 验证关系内部 gRPC 服务实现 |
| `MessageDeliveryPayloadTest.cpp` | 验证消息投递负载结构 |
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
