# MomentsService/ 目录树

> 测试 MomentsService 朋友圈服务核心逻辑，覆盖公开请求/响应 DTO 的 JSON 兼容性与字段清单。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `MomentsOutputDtosTest.cpp` | 验证朋友圈公开请求/响应 DTO 的 JSON 字段、兼容解析、空数组和反射字段清单 |
| `MomentsRouteRegistrationAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入朋友圈路由注册字面量算法。 |
| `MomentsRouteRegistrationAlgorithmsTest.cpp` | 验证朋友圈路由注册 method 和 `/api/moments/*` path 字面量 module-backed helper |
| `MomentsRouteSchemaAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入朋友圈路由 schema 字面量算法。 |
| `MomentsRouteSchemaTest.cpp` | 验证朋友圈稳定路由只读 schema descriptor 与快照输出 |
| `MomentsServiceAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入朋友圈 service facade guard 算法。 |
| `MomentsServiceAlgorithmsTest.cpp` | 验证朋友圈 service facade module 的 endpoint、可见性、认证、ID 和 limit guard |
| `MomentsRelationClientAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入朋友圈关系客户端字段名/RPC 截止/过滤 guard 算法。 |
| `MomentsRelationClientAlgorithmsTest.cpp` | 验证朋友圈关系客户端 module 的请求/响应字段名、RPC 截止时间与 fail-closed 过滤 guard |
| `MomentsGatewayRuntimeAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入朋友圈网关入口运行时字面量/guard 算法。 |
| `MomentsGatewayRuntimeAlgorithmsTest.cpp` | 验证朋友圈网关入口 module 的服务名、网关名、默认端口与 AWS 初始化 guard |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
