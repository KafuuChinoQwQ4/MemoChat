# MemoChat 开发文档

本目录保存可版本化的项目开发文档。`docs/resume/` 和 `docs/superpowers/` 属于个人资料或工作流生成物，按 `.gitignore` 继续忽略；正式开发文档放在本目录的 `dev/`、`architecture/`、`contracts/` 和 `adr/`。

## 阅读入口

| 主题 | 入口 | 适用场景 |
| --- | --- | --- |
| 开发环境与命令 | [dev/README.md](dev/README.md) | 新环境、构建、测试、启动、排障 |
| 项目模块地图 | [architecture/README.md](architecture/README.md) | 了解客户端、服务端、Web、AI、基础设施边界 |
| 跨模块契约 | [contracts/README.md](contracts/README.md) | 改认证、聊天长连接、媒体和文件链路 |
| 架构决策 | [adr/README.md](adr/README.md) | 查看长期约束和不可回退原则 |

## 文档放置规则

- 全项目通用说明放在 `docs/`。
- 模块内部细节放在对应模块的 `docs/`，例如 `apps/server/core/docs/`、`apps/client/desktop/MemoChat-qml/docs/`、`apps/server/core/AIOrchestrator/docs/`。
- 一次性任务计划和修复过程放在 `.ai/`，不要混入正式开发文档。
- 新增、删除、移动文档目录时同步 `_TREE.md`。
