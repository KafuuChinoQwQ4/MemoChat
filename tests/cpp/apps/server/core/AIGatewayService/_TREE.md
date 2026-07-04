# AIGatewayService/ 目录树

> 测试 AIGatewayService AI 网关服务的公共请求与稳定响应 DTO 等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIGatewayRuntimeAlgorithmsTest.cpp` | 验证 AI 网关入口端口和线程数选择 module-backed helper |
| `AIServiceAlgorithmsConsumer.cpp` | 导入 AI 网关服务算法 module 并暴露测试桥接函数 |
| `AIServiceAlgorithmsTest.cpp` | 验证 AI 网关服务响应常量、默认查询值和空内容判定 module-backed helper |
| `AIPublicDtosTest.cpp` | 验证 AI 网关公共请求/稳定响应 DTO 的 JSON 兼容性与字段清单 |
| `AIRouteModuleAlgorithmsConsumer.cpp` | 导入 AI 路由代理算法 module 并暴露测试桥接函数 |
| `AIRouteModuleAlgorithmsTest.cpp` | 验证 AI 路由代理目标匹配和超时下限 module-backed helper |
| `AIRouteRegistrationAlgorithmsConsumer.cpp` | 导入 AI route registration 字面量 module 并暴露测试桥接函数 |
| `AIRouteRegistrationAlgorithmsTest.cpp` | 验证 AI 路由注册 method 和 `/ai/*` path 字面量 module-backed helper |
| `AIRouteSchemaAlgorithmsConsumer.cpp` | 导入 AI route schema 字面量 module 并暴露测试桥接函数 |
| `AIRouteSchemaTest.cpp` | 验证 AI 网关稳定 mapper 路由只读 schema descriptor 与快照输出 |
| `AIGatewayClientAlgorithmsConsumer.cpp` | 导入 AI 网关 gRPC 客户端字面量 module 并暴露测试桥接函数 |
| `AIGatewayClientAlgorithmsTest.cpp` | 验证 AI 网关客户端 AIServer 目标默认值、空字段回退与错误 payload 字面量 module-backed helper |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
