---
name: memochat-task
description: Use when implementing a normal MemoChat feature, bugfix, refactor, or project documentation change that needs context, planning, validation, and review.
---

# MemoChat 任务

用于普通实现工作。先读 `skills/SKILL.md` 和 `skills/rule.md`，再按领域加载聚焦 skill。实现类任务的并发决策归 `skills/parallel-agents.md`。

## .ai 产物

创建或复用：

```text
.ai/<project>/
  about.md
  <letter>/
    context.md
    plan.md
    review1.md
    logs/
      phase-setup.result.md
      phase-verify.result.md
```

如果请求第一个 token 匹配 `.ai/<token>/about.md`，用该项目继续；否则创建简短 kebab-case 项目名和下一个任务字母。

## 工作流

1. `git status --short`，记录会影响任务的已有脏文件；不要回退用户改动。
2. 收集上下文到 `context.md`：任务、相关文件、服务/数据依赖、Docker/MCP 查询、构建/测试命令和风险。
3. 涉及新功能、跨服务契约、数据库/RAG schema、QML 大改、部署或端口风险时，在 `context.md` 或 `plan.md` 写轻量设计：目标/非目标、契约、兼容性边界、验证和回滚/阻塞条件。
4. 编写 `plan.md`：具体文件、函数/模块范围、验证命令、小步骤、并发决策；禁止 `TBD`、`TODO`、泛化占位。
5. 读取 `skills/parallel-agents.md`，决定 Controller + worker 或本地单人；本地单人必须写明原因。
6. 一次实现一个计划阶段；手工编辑使用 `apply_patch`，避免无关格式化。
7. 遇到 bug、测试失败、构建失败或连续修复无效，切到 `skills/debugging.md`。
8. 按改动风险运行最窄验证；需要持久测试或运行时测试循环时读取 `skills/withtest.md`。
9. 完成前读取 `skills/review.md`，检查实际 diff、验证证据和剩余风险。

## 相关区域

- C++ 服务：`apps/server/core/**`
- QML 客户端：`apps/client/desktop/**`、`infra/Memo_ops/client/**`
- 运行时配置：`apps/server/config/**`、`infra/Memo_ops/config/**`、`infra/deploy/local/**`
- 数据库迁移：`apps/server/migrations/postgresql/**`
- 脚本：`tools/scripts/**`、`tools/scripts/status/**`
- 测试：根 `tests/`，负载工具在 `tools/loadtest/python-loadtest`

## Docker 和构建

- 容器是基础设施事实来源；使用 Docker 或 MCP 检查数据库、队列、对象存储、观测和 AI/RAG 依赖。
- 不要修改稳定端口，除非任务明确要求并说明影响。
- Postgres 主机端口通常是 `15432` 到容器 `5432`；Redis 本地开发密码为 `123456`。
- Linux 部署/运行时验证统一使用 `linux-full-gcc16`：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
ctest --preset linux-full-gcc16 --output-on-failure
```

运行时脚本优先使用：

```bash
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
tools/scripts/status/stop-all-services.sh
```

`.bat`/`.ps1` 脚本仅用于旧版 Windows 运行时/客户端检查，或 Linux 服务运行后从 Windows 发起的既有 smoke 探针。

## 完成报告

最终回复必须基于本轮实际证据，包含：修改文件、并发决策、验证命令和结果、测试层覆盖或无法执行原因、适用时的 Docker/MCP 检查、阻塞点/剩余风险，以及后续任务可用的 `.ai` 项目名。不要用“应该通过”“看起来好了”替代证据。
