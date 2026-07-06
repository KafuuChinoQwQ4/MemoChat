# apps/ 目录树

> 镜像主项目 apps/ 的 Python 契约测试层，按 client 与 server 分流。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`client/`](client/_TREE.md) | 桌面客户端契约/结构测试 |
| [`server/`](server/_TREE.md) | 后端服务契约/结构测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_web_moments_masonry_contract.py` | 校验 Web 朋友圈瀑布流内容资源映射与预览截断契约 |
| `test_web_security_contract.py` | 校验 Web 客户端 CSP 与 WebSocket TLS 降级防护契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
