---
name: memochat-debugging
description: Use when MemoChat has a bug, test failure, build failure, unexpected behavior, regression, or repeated failed fix attempt before proposing or applying fixes.
---

# MemoChat 系统调试

核心原则：先找根因，再修复。没有证据的“看起来是”不能进入实现。

## 使用场景

- 测试、构建、部署、smoke 或运行时行为失败。
- UI、服务、数据库、队列、对象存储、AI/RAG 或观测链路表现异常。
- 已经尝试过修复但问题仍然存在。
- 错误跨越多组件，例如客户端 -> GateServer -> ChatServer -> Postgres。

## 四阶段流程

1. 根因调查：
   - 完整读取错误、堆栈、日志和 exit code。
   - 记录可复现步骤；不可复现时先补证据，不猜。
   - 检查最近 diff、配置、环境和 Docker 容器状态。
   - 跨组件问题要在边界处验证输入、输出、配置和状态传播。
2. 模式对比：
   - 查找仓库中相似且正常工作的实现。
   - 对比 broken path 和 working path 的所有差异。
   - 确认依赖、端口、schema、配置和生命周期假设。
3. 单一假设：
   - 写清楚“我认为根因是 X，因为证据 Y”。
   - 用最小命令、日志、查询或代码改动验证一个变量。
   - 假设不成立时回到调查，不叠加多个修复。
4. 根因修复：
   - 能自动化时先写最窄失败测试或复现脚本。
   - 只修根因，不顺手重构。
   - 运行受影响的最窄验证，再按任务要求继续构建、smoke 和边界测试。

## 停止信号

出现以下情况时停止猜测，回到根因调查：

- “先试一下这个改动。”
- “应该是这个原因。”
- “先修了再测。”
- 一次提交里混入多个无关修复。
- 同一问题已经连续修复 2 次仍失败。
- 每次修复都暴露新的共享状态、生命周期或架构问题。

如果同类问题连续 3 次修复仍失败，暂停继续打补丁，重新评估架构、契约或测试环境，并向用户说明证据和选择。

## 记录要求

在 `.ai/<project>/<letter>/context.md`、`plan.md` 或验证日志记录：

- 失败命令和关键输出。
- 已确认的复现步骤。
- 根因假设和证据。
- 被排除的假设。
- 最小修复和重跑验证结果。
