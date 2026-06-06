---
name: parallel-agents
description: Use when a MemoChat implementation task can be split into Controller-led parallel work lines and the current tool policy plus user authorization allow worker agents.
---

# MemoChat 并行代理

这个 skill 只定义 MemoChat 的 Controller/worker 决策、所有权和验收契约。通用并发技巧可参考 `skills/superpowers/dispatching-parallel-agents/SKILL.md` 和 `skills/superpowers/subagent-driven-development/SKILL.md`，但本文件优先。

## 决策规则

- 每个非平凡实现任务都先考虑 Controller 主导并行。
- 真正派发 worker 必须同时满足：当前工具策略允许、用户授权允许、工作线安全、写入范围互不重叠、Controller 已冻结第一版共享契约。
- 极小一行修复、需求无法冻结契约、工作天然顺序、多个 worker 必须改同一文件、或没有有意义测试/复审/运行时工作线时，可以本地单人。
- 本地单人必须在 `plan.md` 记录：

```text
Concurrency decision: local-only because <reason>.
```

- 如果并发有价值但当前会话不能派发 worker，记录：

```text
Concurrency decision: parallel blocked by active tool/user policy; Controller continued local-only.
```

## Controller 职责

Controller 必须负责：

- 架构、目标/非目标、共享契约和兼容性边界。
- `.ai/<project>/<letter>/context.md`、`plan.md`，契约重要时再写 `contracts.md`。
- 工作线拆分、写入所有权、worker 提示词和集成顺序。
- Docker/MCP 假设、稳定端口、测试范围和验证命令。
- 检查 worker 结果、实际 diff、契约一致性和最终验收。

Controller 只在解除集成阻塞时写少量胶水；worker 运行期间不要沉入大范围产品实现。

## 默认工作线

常用上限：1 个 Controller + 最多 5 条 worker。安全时最低形态是 Controller + Tests Worker + 至少一条实现/调查工作线。

| 工作线 | 典型负责 | 常见写入范围 |
| --- | --- | --- |
| Backend | 服务逻辑、API、schema、迁移、配置 | `apps/server/core/**`、`apps/server/config/**`、`apps/server/migrations/**` |
| Frontend | QML、客户端 controller、资源连接 | `apps/client/desktop/**`、`infra/Memo_ops/client/**` |
| Data/AI | AIOrchestrator、RAG、工具契约、模型/向量/图依赖 | `apps/server/core/AIOrchestrator/**`、AI/RAG 配置 |
| Tests | 单元、回归、边界、smoke、负载、验证产物 | `tests/**`、`tools/scripts/**`、`tools/loadtest/**` |
| Integration | 编译修复、跨线连接、运行时 smoke、文档 | Controller 批准的窄范围 |

按任务自然切片命名工作线，例如 `Schema Worker`、`QML Layout Worker`、`Gateway Proxy Worker`。任何非平凡实现默认包含 Tests Worker；跳过时必须说明为什么没有测试面或工具被阻塞。

## Worker 提示词必须包含

- 任务目标和非目标。
- 已读取的必要 skill。
- 当前 `context.md`、`plan.md`、可选 `contracts.md` 路径。
- 允许编辑和禁止编辑的精确文件/目录。
- 预期构建/测试命令。
- Docker-only、稳定端口、`archlinux`、`/data` 和不要回退用户改动的警告。
- 其他代理可能并发编辑仓库。
- 必需结果文件路径。

Tests Worker 还要包含预期行为矩阵、边界情况和需要先添加/更新的测试。

只有构造较大的可复用提示词时才读取 `skills/PROMPTS.md`。

## Worker 输出契约

每个 worker 写入：

```text
.ai/<project>/<letter>/logs/parallel-<lane>.progress.md
.ai/<project>/<letter>/logs/parallel-<lane>.result.md
```

结果文件格式：

```text
STATUS: DONE|BLOCKED|NEEDS_INTEGRATION|NEEDS_DECISION
ROLE: Controller|Backend|Frontend|DataAI|Tests|Integration|Custom
OWNERSHIP: 拥有的文件/目录
CHANGED: 已改路径，或 none
CONTRACTS: API/schema/QML properties/events 变更，或 none
VERIFY: 执行的命令和结果
RISKS: 剩余问题
NEXT: 准确集成步骤
```

结果缺少证据时，Controller 必须拒绝直接验收。

## 集成和验收

Controller 完成前必须：

1. 读取所有 `parallel-*.result.md`。
2. 检查 `git status --short` 和相关 diff。
3. 确认 worker 写入范围没有冲突，且共享契约与最终代码一致。
4. 确认 Tests Worker 已覆盖有意义行为，或 `plan.md` 记录跳过原因。
5. 运行最窄有意义验证；触及 Linux 部署/运行时使用 `linux-full-gcc16`。
6. 写入 `review1.md` 和 `logs/phase-verify.result.md`。
7. 更新 `plan.md` 状态和并发决策。

worker 不得扩展范围做顺手重构、单方面更新共享契约、运行破坏性 git 命令，或未经批准修改 Docker 发布端口。
