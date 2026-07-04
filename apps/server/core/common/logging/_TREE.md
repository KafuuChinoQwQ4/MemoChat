# logging/ 目录树

> 共享的日志与可观测性基础设施：结构化日志、敏感信息脱敏、OpenTelemetry 遥测与 gRPC 链路追踪上下文传播。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 公共日志 C++ modules 接口单元集合。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GrpcTrace.cpp` | gRPC 调用链路追踪实现 |
| `GrpcTrace.h` | gRPC 链路追踪声明 |
| `LogConfig.cpp` | 日志配置初始化实现 |
| `LogConfig.h` | 日志配置声明 |
| `Logger.cpp` | 日志器实现 |
| `Logger.h` | 日志器接口声明 |
| `Redaction.cpp` | 敏感字段脱敏实现 |
| `Redaction.h` | 脱敏规则声明 |
| `Telemetry.cpp` | 遥测（指标/追踪）初始化实现 |
| `Telemetry.h` | 遥测接口声明 |
| `TelemetryConfig.cpp` | 遥测配置实现 |
| `TelemetryConfig.h` | 遥测配置声明 |
| `TraceContext.h` | 追踪上下文传播定义 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
