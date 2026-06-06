---
name: memochat-debugging
description: Use when MemoChat has a bug, test failure, build failure, unexpected behavior, regression, or repeated failed fix attempt before proposing or applying fixes.
---

# MemoChat 系统调试

这个 skill 是 MemoChat 的调试适配层，不重复通用调试方法。

- 默认先读取 `skills/superpowers/systematic-debugging/SKILL.md`，按其根因调查流程执行。
- 用户明确点名 Matt Pocock `/diagnose` 时，再读取 `skills/mattpocock/engineering/diagnose/SKILL.md`。
- 本文件只补充 MemoChat 项目必须保留的证据、边界和记录要求。

## 使用场景

- 测试、构建、部署、smoke 或运行时行为失败。
- UI、服务、数据库、队列、对象存储、AI/RAG 或观测链路表现异常。
- 已经尝试过修复但问题仍然存在。
- 错误跨越多组件，例如客户端 -> GateServer -> ChatServer -> Postgres。

## MemoChat 必查证据

- 最近 diff、配置和 `.ai/<project>/<letter>/plan.md` 是否改变了服务契约、QML 属性、schema、端口或脚本。
- Docker 容器状态和重启历史；不要在 Docker 外安装或启动 Redis/Postgres/Mongo/RabbitMQ/Redpanda 等依赖。
- 相关 MCP 查询结果，尤其是 Postgres、Redis、MongoDB、Neo4j、Qdrant、Redpanda 和对象存储状态。
- 客户端 -> GateServer -> ChatServer -> 数据库/队列这类跨组件路径，在每个边界验证输入、输出、配置和状态传播。
- QML/UI 问题要区分共享路径和平台专用路径，保留已经正常的平台表现。

## 停止信号

出现以下情况时停止猜测，回到根因调查：

- “先试一下这个改动。”
- “应该是这个原因。”
- “先修了再测。”
- 一次提交里混入多个无关修复。
- 同一问题已经连续修复 2 次仍失败。
- 每次修复都暴露新的共享状态、生命周期或架构问题。

如果同类问题连续 3 次修复仍失败，暂停继续打补丁，重新评估架构、契约或测试环境，并向用户说明证据和选择。

## 修复边界

- 能自动化时先写最窄失败测试或复现脚本，再修复。
- 只修根因，不顺手重构；需要架构调整时先说明证据和影响面。
- 运行受影响的最窄验证，再按任务要求继续构建、smoke 和边界/异常测试。
- 触及运行时部署或 Linux 服务行为时，使用 `linux-full-gcc16` 构建路径和现有 `tools/scripts/status/*` 脚本。

## 记录要求

在 `.ai/<project>/<letter>/context.md`、`plan.md` 或验证日志记录：

- 失败命令和关键输出。
- 已确认的复现步骤。
- 根因假设和证据。
- 被排除的假设。
- 最小修复和重跑验证结果。
