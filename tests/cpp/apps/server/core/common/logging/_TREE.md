# logging/ 目录树

> 测试服务端公共日志库，聚焦结构化 JSON 日志输出。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `GrpcTraceAlgorithmsConsumer.cpp` | 测试侧导入 gRPC 链路追踪算法模块并暴露 metadata 注入/绑定 helper 给 GTest 断言。 |
| `JsonLoggerTest.cpp` | 验证 JSON 日志器的字段与格式 |
| `LogConfigAlgorithmsConsumer.cpp` | 测试侧导入日志配置算法模块并暴露 bool token/clamp/校验 helper 给 GTest 断言。 |
| `LogConfigAlgorithmsTest.cpp` | 固定日志配置 bool 词表、max_files/max_size_mb 回退与 rotate/level 校验默认值行为。 |
| `LoggerAlgorithmsConsumer.cpp` | 测试侧导入日志器算法模块并暴露默认目录与顶层字段 helper 给 GTest 断言。 |
| `LoggingConfigTest.cpp` | 验证日志/遥测配置解析的 trim/lower/bool 行为，覆盖 C++ modules 导入后的 memochat_logging 路径。 |
| `RedactionAlgorithmsConsumer.cpp` | 测试侧导入日志脱敏算法模块并暴露轻量 helper 给 GTest 断言。 |
| `TelemetryAlgorithmsConsumer.cpp` | 测试侧导入遥测算法模块并暴露 endpoint、导出守卫和 span 属性 helper 给 GTest 断言。 |
| `TelemetryConfigAlgorithmsConsumer.cpp` | 测试侧导入遥测配置算法模块并暴露 bool token/clamp/默认 protocol helper 给 GTest 断言。 |
| `TelemetryConfigAlgorithmsTest.cpp` | 固定遥测配置 bool 词表、sample_ratio 区间 clamp 与空 protocol 默认值行为。 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
