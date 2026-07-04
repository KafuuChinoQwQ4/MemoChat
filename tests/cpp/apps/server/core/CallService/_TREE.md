# CallService/ 目录树

> 测试 CallService 通话服务核心支撑逻辑，覆盖通话请求与会话缓存 DTO 等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallPublicDtosTest.cpp` | 验证通话公共请求/固定成功响应 DTO 的 JSON 兼容性与字段清单 |
| `CallRouteModuleAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入通话路由注册常量与路由模块算法。 |
| `CallRouteModuleAlgorithmsTest.cpp` | 验证通话 route registration 常量、uid 查询解析与 trace_id 追加边界算法。 |
| `CallRouteResponseAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入通话路由响应元数据字面量算法。 |
| `CallRouteResponseAlgorithmsTest.cpp` | 验证通话路由响应 HTTP 状态码、content type 与 trace_id 探测字面量算法。 |
| `CallRouteSchemaAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入通话路由 schema 字面量算法。 |
| `CallRouteSchemaTest.cpp` | 验证通话生产路由只读 schema descriptor 与快照输出 |
| `CallServiceAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入通话服务门面状态与守卫算法 |
| `CallServiceAlgorithmsTest.cpp` | 验证通话服务门面配置、状态、事件、TTL 与 token 角色算法 |
| `CallSessionCacheDtoTest.cpp` | 验证通话会话缓存 DTO 的 JSON 编解码与字段清单 |
| `CallSessionMathAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入通话会话时间/时长计算算法 |
| `CallSessionMathAlgorithmsTest.cpp` | 验证通话会话过期时间、房间短 id、时长计算与通知消息 id 算法 |
| `CallTokenAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入通话 token 编码算法 |
| `CallTokenAlgorithmsTest.cpp` | 验证通话 token Base64URL module 算法 |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
