# tools/ 目录树

> 镜像主项目 tools/ 的 Python 测试，覆盖压测工具与运维脚本契约。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`loadtest/`](loadtest/_TREE.md) | 压测工具测试 |
| [`release_integration/`](release_integration/_TREE.md) | 发布 preset 与 C++ bundle 的跨模块集成契约测试 |
| [`release_security/`](release_security/_TREE.md) | 发布树扫描、环境隔离和 CI/CD 交接安全契约测试 |
| [`scripts/`](scripts/_TREE.md) | 运维脚本（桌宠/状态部署）契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_tool_secret_externalization_contract.py` | 工具/MCP/压测脚本的密钥外置化契约测试 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
