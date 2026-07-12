# AIServer/ 目录树

> 测试 AIServer 服务，覆盖 AI 服务核心算法、JSON/protobuf 映射与仓储判定。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIServiceClientAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入 AIServer AIOrchestrator HTTP 客户端路径、SSE 和默认值算法 |
| `AIServiceClientAlgorithmsTest.cpp` | 验证 AIServer HTTP 客户端 module 的路径常量、URL 编码、SSE 和 fallback 决策 |
| `AIServiceCoreAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入 AIServer 核心服务响应码、分页和会话校验算法 |
| `AIServiceCoreAlgorithmsTest.cpp` | 验证 AIServer 核心服务 module 的响应码、角色字面量、分页和校验决策 |
| `AIServiceImplAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入 AIServer gRPC service implementation 的 trace span 名称 |
| `AIServiceImplAlgorithmsTest.cpp` | 验证 AIServer gRPC service implementation module 的 trace span 名称与 RPC 类型字面量 |
| `AIServiceAlgorithmsTest.cpp` | 验证 AIServer module-backed 算法 wrapper 的成功判定与 limit 归一化 |
| `AIServiceJsonMapperTest.cpp` | 验证 AI 服务 JSON/protobuf 映射与 DTO 字段清单 |
| `AIServerRepositoryAlgorithmsConsumer.cpp` | 测试专用 module consumer，导入 AIServer repository 默认值与结果判断算法 |
| `AIServerRepositoryAlgorithmsTest.cpp` | 验证 AIServer repository schema 默认值、会话标题默认值和结果判断算法 |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
