# core/ 目录树

> GateShared 的核心基础设施层：缓存、下游客户端、配置、持久化、运行时与跨模块支撑组件。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cache/`](cache/_TREE.md) | Redis 缓存管理与管线。 |
| [`clients/`](clients/_TREE.md) | 对 Auth/Chat/Verify 等服务的 gRPC/RPC 客户端。 |
| [`config/`](config/_TREE.md) | 配置管理与常量。 |
| [`persistence/`](persistence/_TREE.md) | Mongo/Postgres 数据访问层。 |
| [`runtime/`](runtime/_TREE.md) | IO 线程池、全局对象与单例工具。 |
| [`support/`](support/_TREE.md) | 跨模块支撑（用户 token 校验等）。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | core 子库的构建目标定义。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
