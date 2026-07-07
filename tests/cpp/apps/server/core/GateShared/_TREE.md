# GateShared/ 目录树

> GateShared 共享路由/runtime 层的 C++ 测试。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `BearerAccessAuthAlgorithmsConsumer.cpp` | 在测试目标中直接 import Bearer access token 解析算法 module，暴露 header 匹配和 token 定位给 GTest。 |
| `BearerAccessAuthAlgorithmsTest.cpp` | 验证 Authorization Bearer 头解析算法的大小写匹配、空白处理和旧 scheme 拒绝。 |
| `CMakeLists.txt` | 注册 GateShared 测试目标的 CMake 配置。 |
| `ChatGrpcClientAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate Chat gRPC 客户端算法 module，暴露连接池、span 和 RPC guard 给 GTest。 |
| `ChatGrpcClientAlgorithmsTest.cpp` | 验证 Gate Chat gRPC 客户端算法 module 的连接池默认值、span 元数据和 RPC 失败 guard。 |
| `CrossModuleRouteSchemaCatalogTest.cpp` | 用全部 7 个真实路由模块的 `RouteSchemas()` 构建目录，断言无跨模块 `method+path` 冲突，并用注入的重复路由验证守卫确实生效。 |
| `CServerLifetimeTest.cpp` | 验证 GateShared CServer accept 循环生命周期与 shared_ptr 续命。 |
| `GateDomainRuntimeTest.cpp` | 验证 GateDomain runtime module-backed 端口和线程选择 wrapper。 |
| `GateHttpJsonSupportAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate HTTP JSON 支持算法 module，暴露 content-type、trace 与解析 guard 给 GTest。 |
| `GateHttpJsonSupportAlgorithmsTest.cpp` | 验证 Gate HTTP JSON 支持算法 module 的响应元数据、解析失败与 trace-id guard。 |
| `GateWorkerPoolAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate worker 线程池算法 module，暴露线程数选择和生命周期 guard 给 GTest。 |
| `GateWorkerPoolAlgorithmsTest.cpp` | 验证 Gate worker 线程池算法 module 的线程数选择和 join/stop guard。 |
| `H3LegacyRouteAlgorithmsConsumer.cpp` | 在测试目标中直接 import H3 legacy 路由算法 module，暴露 GET/POST 路径和未命中响应元数据给 GTest。 |
| `H3LegacyRouteAlgorithmsTest.cpp` | 验证 H3 legacy 路由算法 module 的路由表顺序、唯一性和 404 响应元数据。 |
| `H1RouteAdapterAlgorithmsConsumer.cpp` | 在测试目标中直接 import HTTP/1 路由适配器算法 module，暴露连接/响应 guard 给 GTest。 |
| `H1RouteAdapterAlgorithmsTest.cpp` | 验证 HTTP/1 路由适配器算法 module 的连接字段、content-type 与文件响应 guard。 |
| `HealthRouteAlgorithmsConsumer.cpp` | 在测试目标中直接 import 健康检查路由算法 module，暴露 probe 路径和响应元数据给 GTest。 |
| `HealthRouteModuleTest.cpp` | 验证健康检查路由模块通过真实 RouteRegistry 注册 `/healthz` 与 `/readyz` 响应。 |
| `ModuleProbeTest.cpp` | 验证 GateRuntimeCore 内的 C++ module 探针可被链接并运行。 |
| `MongoDaoAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate Mongo DAO 算法 module，暴露配置和动态内容 guard 给 GTest。 |
| `MongoDaoAlgorithmsTest.cpp` | 验证 Gate Mongo DAO 算法 module 的启用开关、默认 collection、配置和 moment_id guard。 |
| `PostgresDaoAccountAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate Postgres 账户 DAO 算法 module，暴露注册和账户查询 guard 给 GTest。 |
| `PostgresDaoAccountAlgorithmsTest.cpp` | 验证 Gate Postgres 账户 DAO 算法 module 的默认头像、public-id retry 与查询 guard。 |
| `PostgresDaoAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate Postgres DAO 算法 module，暴露默认值和 guard 给 GTest。 |
| `PostgresDaoAlgorithmsTest.cpp` | 验证 Gate Postgres DAO 算法 module 的 schema/sslmode 默认值、pool size clamp 与账户库 guard。 |
| `PostgresMgrAlgorithmsConsumer.cpp` | 在测试目标中直接 import Gate Postgres 管理器算法 module，暴露跨域转发面计数给 GTest。 |
| `PostgresMgrAlgorithmsTest.cpp` | 验证 Gate Postgres 管理器算法 module 的账户、通话、媒体、朋友圈转发面计数。 |
| `RedisPipelineAlgorithmsConsumer.cpp` | 在测试目标中直接 import Redis pipeline 算法 module，暴露登录缓存 key 前缀给 GTest。 |
| `RedisPipelineAlgorithmsTest.cpp` | 验证 Redis pipeline 算法 module 的登录缓存 key 前缀常量。 |
| `RedisMgrAlgorithmsConsumer.cpp` | 在测试目标中直接 import Redis 管理器算法 module，暴露连接池和 reply 判定 helper 给 GTest。 |
| `RedisMgrAlgorithmsTest.cpp` | 验证 Redis 管理器算法 module 的连接池默认值、reply 判定和批量 guard。 |
| `RouteRegistryTest.cpp` | 验证 GateRuntimeCore 内 RouteRegistry 的 exact/prefix 分发行为，覆盖导入模块后的真实路由算法路径。 |
| `RouteSchemaCatalogTest.cpp` | 验证路由 schema 目录聚合：模块/路由顺序、文本与 JSON 渲染（含转义）及跨模块重复路由检测。 |
| `RouteSchemaTest.cpp` | 验证路由 DTO schema helper 的反射字段清单、无 body 路由与 snapshot 输出。 |
| `UserTokenValidatorAlgorithmsConsumer.cpp` | 在测试目标中直接 import 用户 access token 校验算法 module，暴露前置 guard 和 Redis value 接受判断给 GTest。 |
| `UserTokenValidatorAlgorithmsTest.cpp` | 验证用户 access token 校验算法 module 的 JWT/current-token guard 与 Redis 命中判断。 |
| `main.cpp` | GTest 测试入口。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
