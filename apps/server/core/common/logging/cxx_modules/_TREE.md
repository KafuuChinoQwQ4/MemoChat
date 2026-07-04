# cxx_modules/ 目录树

> common/logging 的 C++ modules 接口单元，当前承载日志/遥测配置解析纯算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GrpcTrace.cppm` | 导出 gRPC trace metadata 注入和绑定时的空值补全纯算法模块。 |
| `LoggingConfig.cppm` | 导出日志与遥测配置解析共享的 ASCII trim/lower 算法模块。 |
| `LogConfig.cppm` | 导出日志配置的 bool token 词表、max_files/max_size_mb 非正回退与 rotate/level 校验默认值纯算法模块。 |
| `Logger.cppm` | 导出日志器默认目录选择与顶层字段名判定纯算法模块。 |
| `Redaction.cppm` | 导出日志脱敏敏感 key 分类、token/email 遮罩边界等纯算法模块。 |
| `Telemetry.cppm` | 导出遥测 endpoint 解析、服务名默认值和 span 导出守卫等纯算法模块。 |
| `TelemetryConfig.cppm` | 导出遥测配置的 bool token 词表、sample_ratio 区间 clamp 与空 protocol 默认值纯算法模块。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
