# common/ 目录树

> 跨所有服务端微服务共享的基础库：雪花 ID、跨平台/兼容垫片、认证密钥、集群发现、数据库/JSON 兼容层、日志与遥测、内部 proto、运行时配置等。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`auth/`](auth/_TREE.md) | 认证密钥与跨服务登录票据定义 |
| [`cluster/`](cluster/_TREE.md) | 集群节点发现（含 Etcd 实现） |
| [`db/`](db/_TREE.md) | 数据库（pqxx）兼容垫片 |
| [`json/`](json/_TREE.md) | Glaze JSON 序列化兼容层 |
| [`logging/`](logging/_TREE.md) | 日志、脱敏与 OpenTelemetry 遥测 |
| [`proto/`](proto/_TREE.md) | 跨服务共享的 gRPC/protobuf 定义 |
| [`reflection/`](reflection/_TREE.md) | C++26 标准反射编译期字段元信息辅助层 |
| [`runtime/`](runtime/_TREE.md) | 运行时配置与 IO 上下文池等基础设施 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `BoostStubs.cpp` | Boost 相关符号的存根实现 |
| `SnowflakeUtil.h` | 雪花算法分布式 ID 生成工具封装 |
| `StdFix.h` | 标准库跨平台兼容修正 |
| `WinSdkCompat.h` | Windows SDK 兼容垫片 |
| `WinsockCompat.h` | Winsock 套接字 API 兼容垫片 |
| `snowflake.hpp` | 雪花 ID 算法底层实现 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
