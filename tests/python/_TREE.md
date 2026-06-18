# python/ 目录树

> Python 测试树根：以 unittest 编写的静态结构/契约测试（扫描源码与配置，不构建、不起服务），子树镜像主项目相对路径。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`apps/`](apps/_TREE.md) | 客户端与服务端应用的契约测试，镜像 apps/ |
| [`infra/`](infra/_TREE.md) | 运维/部署相关测试，镜像 infra/ |
| [`support/`](support/_TREE.md) | 测试共享支撑工具（仓库根定位等） |
| [`tools/`](tools/_TREE.md) | 工具脚本/压测相关测试，镜像 tools/ |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 将 python 标记为测试包 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
