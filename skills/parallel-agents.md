---
name: parallel-agents
description: Use when a MemoChat implementation task can be split into Controller-led parallel work lines and the current tool policy plus user authorization allow worker agents.
---

# MemoChat 并行代理

在读取 `skills/SKILL.md` 和相关聚焦 skill 之后，每个 MemoChat 实现任务都使用这个 skill 做并发决策。项目默认形态是 **Controller 主导 + 可拆分 worker**，但真正派发 worker 必须同时满足：当前工具策略允许、用户授权允许、工作线安全且互不重叠。Controller 收集到足够上下文并冻结第一版共享契约后，只要这些条件成立，就立即派发 worker。

这个 skill 只管并发决策，不替代 `task.md`、`withtest.md` 或 `review.md`。如果你只是判断要不要开 worker，读到“并行化启发”通常就够了；只有要构造 worker 提示词、接收 worker 结果或做最终验收时，才继续读后面的契约和验收门。

单代理执行是例外，不是便利选项。极小的一行修复，只有在不存在有用的测试、复审、运行时或文档工作线时才可以保持单代理，并且实现前必须在 `plan.md` 中明确记录。任何任务只要在上下文收集、架构、后端、前端、数据库、测试、运行时验证、文档或复审中涉及超过一项，都必须先设计 Controller 主导的并行模型。如果 worker 派发被当前工具、平台策略或用户授权条件阻塞，记录 `Concurrency decision: parallel blocked by active tool/user policy; Controller continued local-only`，并保持其余工作流不变。

## 不可协商的形态

每个并行任务都有一个 **Controller agent**。

Controller 是必需的，并负责：

- 架构设计和非目标
- `.ai/<project>/<letter>/context.md`
- `.ai/<project>/<letter>/plan.md`
- 工作线拆分和写入所有权
- 共享契约、DTO/API/QML 属性名、迁移、功能开关和配置边界
- worker agent 提示词构造
- 集成顺序和冲突策略
- 最终 diff 复审和整体验收
- 最终验证决策和剩余风险摘要

Controller 只应在能解除集成阻塞时实现少量胶水改动。worker 运行期间，Controller 不应沉入大范围产品代码实现。

## 默认代理拓扑

使用一个 Controller 加最多五条 worker 工作线。工作线更少也可以，但并发工作必须始终存在 Controller。对于常规实现任务，安全且互不重叠的写/读范围存在时，最低默认拓扑是 Controller + Tests Worker + 至少一条实现或调查工作线。如果没有启动 worker，`plan.md` 必须说明为什么实现/调查工作线和 Tests Worker 都不可能或无用。

任何非平凡实现任务，默认包含一个专用 **Tests Worker**。这条工作线在实现推进时持续维持测试，并在集成后继续扩展实际行为的单元、smoke、回归和边界覆盖，包括无效输入、空状态、重复操作、生命周期切换、并发/重试情况、持久化往返，以及 UI/API 契约不匹配。仅当任务确实很小或不存在测试面时才跳过，并在 `plan.md` 中记录原因。

| 角色 | 协作层 | 负责内容 | 常见写入范围 |
| --- | --- | --- | --- |
| Controller | 编排 + 验收 | 架构、计划、契约、提示词、合并顺序、复审、最终验收 | `.ai/<project>/<letter>/**`、少量集成胶水 |
| Backend Worker | 执行 | 服务逻辑、API 路由、schema、迁移、配置 | `apps/server/core/**`、`apps/server/config/**`、`apps/server/migrations/**` |
| Frontend Worker | 执行 | QML/客户端 controller UX 和资源连接 | `apps/client/desktop/**`、`infra/Memo_ops/client/**` |
| Data/AI Worker | 执行 | AIOrchestrator、RAG、模型/工具契约、Docker 支撑的 AI 依赖 | `apps/server/core/AIOrchestrator/**`、AI/RAG 配置 |
| Tests Worker | 反馈 + 质量门 | 单元测试、smoke 探针、负载脚本、边界/回归覆盖、验证产物 | 写入 `tests/cpp/<主项目相对路径>/**`、`tests/python/<主项目相对路径>/**`、`tests/fixtures/**`、`tools/scripts/**`、`tools/loadtest/**`；`apps/**/tests/**` 仅作历史上下文读取，除非任务是把它迁到根 `tests/` |
| Integration Worker | 运行时/复审 | 编译修复、跨工作线连接、运行时 smoke、文档 | Controller 批准的窄范围文件 |

当任务有不同的自然切片时，按所有权重命名工作线，而不是按身份命名。示例：`Schema Worker`、`QML Layout Worker`、`Gateway Proxy Worker`、`Observability Worker`。

## 必需派发模式

1. Controller 首先在主线程启动。
2. Controller 读取足够的代码和 Docker/MCP 状态，以创建有用的上下文包。
3. Controller 编写或更新：
   - `.ai/<project>/<letter>/context.md`
   - `.ai/<project>/<letter>/plan.md`
   - 契约重要时的 `.ai/<project>/<letter>/contracts.md`
4. Controller 在任何 worker 修改文件前冻结第一版契约切片。
5. 当安全工作线存在且允许启动时，Controller 默认用自包含提示词派发独立 worker。
6. 对非平凡实现，Controller 派发 Tests Worker，让测试与产品改动并行推进。仅在没有有意义测试面、任务确实很小，或 worker 派发被当前策略阻塞时跳过；原因记录到 `plan.md`。
7. worker 运行期间，Controller 在本地继续做有用且不重叠的工作。
8. worker 通过结果文件和最终消息返回结果。
9. Controller 集成实现工作线，然后在需要额外的集成后覆盖或回归检查时，将最终行为/diff 提供给 Tests Worker。
10. Controller 检查实际 diff、集成、验证、复审，并接受任务或发送定向后续工作。

不要为眼前的阻塞任务启动 worker。如果下一步本地操作依赖某个答案，Controller 先在本地处理该答案，然后再派发并行工作线。

## 并行化启发

出现以下任一情况时，默认派发并行 worker：

- 后端和前端可以通过稳定 API/QML 契约定义
- 测试可以在约定行为上编写，同时实现继续推进
- Tests Worker 可以围绕稳定契约扩展边界情况、生命周期情况、重复操作检查和回归覆盖
- 一条工作线可以检查 Docker/MCP/数据状态，另一条同时编辑代码
- 文档或 `.ai` 产物可以在产品代码实现期间并行更新
- 复审/运行时 smoke 可以从部分工作线输出开始，而无需发明新范围

仅当以下例外之一成立，并且实现前已记录到 `plan.md` 时，才保持本地单人工作：

- 任务是一行或单符号编辑
- 当前工具/策略环境或用户授权条件禁止为当前请求启动 worker
- 用户明确要求单代理/仅本地执行
- 需求太不清楚，无法冻结任何契约
- 两个 worker 会同时需要编辑同一个文件
- 迁移或端口/配置变更需要用户先做决定
- 工作本质上是顺序的，并行代理只会增加合并风险
- 请求的变更没有有意义的测试、smoke 或静态检查面

保持本地单人时，在 `plan.md` 中加入简短说明：

```text
Concurrency decision: local-only because <reason>.
```

如果项目并行模型有价值但当前会话不能派发 worker，使用：

```text
Concurrency decision: parallel blocked by active tool/user policy; Controller continued local-only.
```

此时 Controller 仍要在 `plan.md` 中写出本来会拆出的工作线、所有权和被阻塞原因，并用本地检查清单覆盖 Tests Worker/Review Worker 的关键职责。

## Worker 提示词契约

每个 worker 提示词必须包含：

- 用一段话说明任务目标
- Controller 已经考虑过的必需 skill 文件
- 当前 `.ai/<project>/<letter>/context.md`、`plan.md` 和 `contracts.md` 路径
- 精确所有权：worker 可以编辑的文件/目录
- worker 不得触碰的文件/目录
- 该工作线预期的构建/测试命令
- Docker/MCP 规则和稳定端口警告
- 说明其他代理可能并发编辑仓库
- 指示不要回退用户或其他代理的改动
- 必需结果文件路径

Tests Worker 提示词还必须包含预期行为矩阵和已知边界情况。要求该工作线先添加或更新测试再运行测试，然后明确报告未覆盖风险。对于有状态流程，应在相关时覆盖创建/读取/更新/删除，重复启动/停止或重启路径，固定配置不变量，随机化边界，空集合和最大支持集合，无效 payload，持久化重载，以及 API/UI 不匹配场景。

只有构造较大的可复用提示词时才使用 `skills/PROMPTS.md`；否则简洁的自包含提示词就足够。

## Worker 输出契约

每个 worker 写入：

- `.ai/<project>/<letter>/logs/parallel-<lane>.progress.md`
- `.ai/<project>/<letter>/logs/parallel-<lane>.result.md`

每个结果必须包含：

```text
STATUS: DONE|BLOCKED|NEEDS_INTEGRATION|NEEDS_DECISION
ROLE: Controller|Backend|Frontend|DataAI|Tests|Integration|Custom
OWNERSHIP: 拥有的文件/目录
CHANGED: 已改路径，或 none
CONTRACTS: API/schema/QML properties/events 变更
VERIFY: 执行的命令和结果
RISKS: 剩余问题
NEXT: 准确集成步骤
```

如果 worker 结果缺少足够证据，Controller 可以拒绝集成。

`parallel-*.md` 是并发工作线产物，不是测试循环产物。它们用于 Controller 汇总、集成和复审；如果任务还启用了 `skills/withtest.md`，则 `test<N>.md` / `result<N>.md` / `fix<N>.md` 继续作为独立的运行时测试循环记录。

## Controller 验收门

Controller 不能在完成以下事项前标记任务完成：

1. 读取所有使用过的 `parallel-*.result.md` 文件。
2. 检查 `git status` 和相关 `git diff`。
3. 确认 worker 写入范围没有意外冲突。
4. 确认共享契约与最终代码一致。
5. 确认 Tests Worker 已运行并扩展有意义覆盖，或 `plan.md` 记录了为什么单独测试工作线没有价值。
6. 运行最窄的有意义验证；触及 Linux 部署/运行时行为时，使用 `linux-full-gcc16`。只有明确的旧版 Windows 验证才使用 `msvc2022-full`。
7. 写入 `.ai/<project>/<letter>/review1.md`。
8. 将验证记录到 `.ai/<project>/<letter>/logs/phase-verify.result.md`。
9. 更新 `plan.md` 状态和并发决策。

如果任何验收门失败，Controller 要么本地修复一个窄问题，要么给拥有该范围的 worker 发送一个定向后续任务；当决定属于产品层面或具有破坏性时，询问用户。

## 工作线所有权规则

- 保持写入所有权互不重叠。
- 如果两条工作线需要同一文件，Controller 指定一个 owner。另一条工作线在结果文件中写说明或补丁建议。
- worker 不得扩展范围做“顺手”重构。
- worker 不得单方面更新共享契约。它们向 Controller 提议变更。
- worker 不得运行破坏性 git 命令。
- 除非用户明确要求且 Controller 批准，否则 worker 不得修改 Docker 发布端口。

## Docker 和运行时规则

基础设施真相来自 Docker 和已配置的 MCP 工具。

一条工作线在依赖数据存储、队列、对象存储、观测服务或 AI/RAG 依赖前，应检查相关容器或 MCP 状态，并将命令/查询记录到上下文或结果日志。

稳定端口必须保持稳定。如果拟定计划需要修改发布端口，停止并询问用户。

## 推荐工作线拆分

API + UI 工作：

- Controller：端点/QML 契约、所有权、最终复审
- Backend Worker：服务端/API/schema
- Frontend Worker：QML/客户端 controller
- Tests Worker：API smoke 和 UI/静态检查
- Integration Worker：构建/运行时验证

AI/RAG 工作：

- Controller：架构、模型/工具契约、数据流
- Data/AI Worker：AIOrchestrator/RAG/工具变更
- Backend Worker：Gate/AIServer 代理或 C++ 契约变更
- Tests Worker：离线测试和 Docker 支撑探针
- Integration Worker：运行时 smoke 和观测检查

数据库/迁移工作：

- Controller：schema 设计、迁移顺序、回滚/兼容性风险
- Backend Worker：DAO/服务变更
- Data Worker：迁移/初始化脚本和 Docker 验证
- Tests Worker：幂等性和查询探针
- Integration Worker：构建和运行时 smoke

QML-heavy 工作：

- Controller：UX 契约和客户端/服务端数据需求
- Frontend Worker：QML 布局/组件/资源
- 仅当 API 数据变化时使用 Backend/Data Worker
- Tests Worker：可用时执行 `qmllint`、资源检查、截图
- Integration Worker：客户端构建和 smoke

## 可复用模板

将 `.ai/parallel-harness-template/` 作为重复并行工作的默认脚手架：

- `about.md` 说明 Controller 主导的并发模型。
- `prompt.md` 是基础派发提示词。
- `tasks.json` 包含 Controller 和 worker 工作线。

在真实任务中使用模板时，将其复制或改写到 `.ai/<project>/<letter>/`，并用实际契约、所有权和验证命令替换占位符。

## 完成清单

最终回复前：

- Controller 结果存在，或 Controller 职责已记录在 `plan.md`
- 所有使用过的 worker 结果文件都存在
- 契约和最终代码一致
- 计划状态反映已完成和延后工作
- 验证命令和结果已记录
- 没有意外的 Docker 端口/配置漂移
- 最终回答说明修改文件、验证、阻塞点和 `.ai` 项目路径
