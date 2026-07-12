# AccountShared/ 目录树

> 测试 AccountShared 账号共享库，覆盖登录缓存 DTO 等账号核心支撑逻辑。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthLocalFallbackCounterStoreTest.cpp` | 验证 Redis 故障本地限流 store 的容量、过期淘汰与并发计数 |
| `AuthCacheAlgorithmsConsumer.cpp` | 直接 import 认证缓存 module，向测试暴露 Redis key 前缀字面量 |
| `AuthCacheAlgorithmsTest.cpp` | 验证认证缓存 module 导出的 Redis key 前缀常量 |
| `AuthLoginCacheProfileDtoTest.cpp` | 验证登录资料缓存 DTO 的 JSON 编解码与字段清单 |
| `AuthLoginSupportTest.cpp` | 验证登录版本门槛通过 module 化 SemVer 算法保持行为 |
| `AuthPublicDtosTest.cpp` | 验证账号公开接口请求 DTO 的兼容解析与字段清单 |
| `AuthRouteRegistrationAlgorithmsConsumer.cpp` | 直接 import 认证 route registration module，向测试暴露 HTTP 方法和路径常量 |
| `AuthRouteRegistrationAlgorithmsTest.cpp` | 验证认证 focused service route registration module 导出的稳定方法和路径常量 |
| `AuthRouteSchemaAlgorithmsConsumer.cpp` | 直接 import 认证 route schema module，向测试暴露常量 helper |
| `AuthRouteSchemaTest.cpp` | 验证认证稳定路由只读 schema descriptor 与快照输出 |
| `AuthServiceAlgorithmsConsumer.cpp` | 直接 import 认证服务 module，向测试暴露响应元数据、uid 守卫、传输选择与 refresh token 发布策略 helper |
| `AuthServiceAlgorithmsTest.cpp` | 验证认证服务 module 导出的响应元数据、注册 uid 守卫、传输选择与 Web refresh token 发布边界 |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `GateAsyncSideEffectAlgorithmsConsumer.cpp` | 直接 import 账号 Kafka 事件 module，向测试暴露 topic/错误字面量 helper |
| `GateAsyncSideEffectAlgorithmsTest.cpp` | 验证账号 Kafka 事件 module 导出的 topic、事件类型与错误字面量 |
| `GateAsyncSideEffectDtosTest.cpp` | 验证账号 Kafka 事件 DTO 的 JSON 编解码与字段清单 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `ProfileRouteRegistrationAlgorithmsConsumer.cpp` | 直接 import 资料 route registration module，向测试暴露 HTTP 方法和路径常量 |
| `ProfileRouteRegistrationAlgorithmsTest.cpp` | 验证资料 route registration module 导出的稳定方法和路径常量 |
| `ProfileRouteSchemaAlgorithmsConsumer.cpp` | 直接 import 资料 route schema module，向测试暴露常量 helper |
| `ProfileRouteSchemaTest.cpp` | 验证资料路由只读 schema descriptor 与快照输出 |
| `ProfileSupportAlgorithmsConsumer.cpp` | 直接 import 资料支撑 module，向测试暴露成功/错误码、错误信息、Redis key 前缀与 uid 守卫 helper |
| `ProfileSupportAlgorithmsTest.cpp` | 验证资料支撑 module 导出的成功/错误码、错误信息、Redis key 前缀与 uid 守卫常量 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
