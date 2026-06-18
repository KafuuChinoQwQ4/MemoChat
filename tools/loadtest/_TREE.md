# loadtest/ 目录树

> 负载测试工具集，分别提供 k6 与 Python 两套压测实现，外加跨实现的分层套件编排入口。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`k6/`](k6/_TREE.md) | 基于 k6 的 HTTP 压测脚本 |
| [`python-loadtest/`](python-loadtest/_TREE.md) | 基于 Python 的分层压测套件 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `run-layered-suite.ps1` | 编排运行分层压测套件的入口脚本 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
