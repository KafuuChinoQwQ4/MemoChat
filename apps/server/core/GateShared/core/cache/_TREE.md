# cache/ 目录树

> GateShared 的 Redis 缓存层：连接管理与批量管线操作。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | Redis 缓存层的 C++ module 接口分组 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `RedisMgr.cpp` | Redis 连接与命令管理实现。 |
| `RedisMgr.h` | Redis 管理接口声明。 |
| `RedisPipelines.cpp` | Redis 批量/管线操作实现。 |
| `RedisPipelines.h` | Redis 管线接口声明。 |
| `CacheReadinessProbes.cpp` | Redis 启动就绪探针工厂实现（仅本切片引用 RedisMgr 单例）。 |
| `CacheReadinessProbes.hpp` | 缓存层就绪探针工厂声明，供各服务 main() 注入到共享启动流程。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
