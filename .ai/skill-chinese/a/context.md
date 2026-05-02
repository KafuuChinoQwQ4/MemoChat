# 上下文

## 任务

用户要求“把所有 skill 改成中文，agent.md 也改中文说明”。本次按仓库实际文件将 `AGENTS.md` 和 `skills/*.md` 全部本地化为中文。

## 相关文件

- `AGENTS.md`：仓库级代理说明，需要中文化。
- `skills/SKILL.md`：主 MemoChat 任务工作流，按项目规则必须先读。
- `skills/task.md`：常规实现工作流，本次作为相关 focused skill 读取。
- `skills/docker-diagnose.md`、`skills/db-migration.md`、`skills/runtime-smoke.md`、`skills/observability.md`、`skills/ai-rag.md`、`skills/qml-ui.md`、`skills/withtest.md`、`skills/planner.md`、`skills/reflect.md`、`skills/release.md`、`skills/icon.md`、`skills/PROMPTS.md`：用户要求的所有 skill 文档。

## 数据和服务依赖

无运行时数据变更。未触碰 Docker compose、数据库、队列、对象存储或 AI/RAG 配置。

## Docker/MCP

本次为纯文档翻译，没有执行 Docker/MCP 查询。端口和命令示例保持原样。

## 验证建议

- 使用 `rg` 搜索明显残留英文标题或说明。
- 使用 `git diff -- AGENTS.md skills` 复查语义是否保持一致。

## 风险

- `skills/PROMPTS.md` 中的状态枚举、路径占位符和命令需要保持稳定，翻译时已避免改动这些机器可读片段。
