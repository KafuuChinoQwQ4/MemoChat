# Memo_ops/ 目录树

> 自研运维监控平台 MemoOps：Python 采集/分析服务端 + QML 桌面客户端 + k8s 部署清单，对 MemoChat 各服务做监控、日志、压测与运维。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`artifacts/`](artifacts/_TREE.md) | 平台产物输出（索引、压测结果、日志）。 |
| [`bin/`](bin/_TREE.md) | MinIO 等依赖的启停脚本。 |
| [`client/`](client/_TREE.md) | MemoOps QML 桌面运维客户端源码。 |
| [`config/`](config/_TREE.md) | 采集器/服务端/客户端配置文件。 |
| [`docs/`](docs/_TREE.md) | 平台文档。 |
| [`k8s/`](k8s/_TREE.md) | MemoOps 及被监控服务的 k8s 清单与 overlay。 |
| `runtime/` | 运行期数据目录（运行时生成，未纳入文档）。 |
| [`scripts/`](scripts/_TREE.md) | 平台初始化、数据导入与启停脚本。 |
| [`server/`](server/_TREE.md) | Python 服务端（采集器、公共逻辑、API 服务）。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 客户端等 C++ 部分的 CMake 构建入口。 |
| `__init__.py` | Python 包标识。 |
| `requirements.txt` | Python 依赖清单。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
