---
name: memochat-mattpocock-skills
description: Use when a MemoChat task should apply project-local Matt Pocock skills that fill gaps beyond Superpowers, especially architecture improvement, domain-language grilling, codebase maps, prototypes, PRDs, issue slicing, handoffs, or compressed communication.
---

# MemoChat Matt Pocock Skills 本地快照

本目录是 `mattpocock/skills` 的项目内快照。

```text
Source: https://github.com/mattpocock/skills
Commit: aaf2453fbdfe7a15c07f11d861224f34ab4b53cb
Snapshot date: 2026-06-05
```

## 使用边界

- 先读 `skills/SKILL.md` 和 `skills/rule.md`，再读本入口。
- 本目录补充 Superpowers，不替代 MemoChat 项目规则或 `skills/superpowers/SKILL.md`。
- 不要全量读取 `skills/mattpocock/**`；只读取当前任务触发的子 skill。
- 子 skill 中若提到 Claude Code slash command、hooks、sub-agents、issue tracker 或安装器，按当前 Codex 工具和 MemoChat 项目约束等价执行。
- Matt Pocock skills 常假设存在 `CONTEXT.md` 和 `docs/adr/`。MemoChat 目前没有这些文件时，只在 `grill-with-docs`、`improve-codebase-architecture` 或用户明确要求持久化领域语言/ADR 时惰性创建。
- 默认不向 GitHub/GitLab 发布 issue 或 PRD；若未配置 issue tracker，先使用 `.ai/<project>/` 或用户指定的本地 markdown 产物。

## 优先触发：Superpowers 缺口

- `skills/mattpocock/engineering/grill-with-docs/SKILL.md`：用于用户想压力测试方案、统一领域语言、沉淀 `CONTEXT.md` 或 ADR；比 Superpowers `brainstorming` 更偏“术语和文档决策”。
- `skills/mattpocock/engineering/improve-codebase-architecture/SKILL.md`：用于代码防腐化、查找架构退化、深模块机会、耦合收敛、测试面改善或提升 AI 可导航性。
- `skills/mattpocock/engineering/zoom-out/SKILL.md`：用于不熟悉某段代码，需要先画出模块、调用者、数据流和更高层上下文。
- `skills/mattpocock/engineering/prototype/SKILL.md`：用于需要一次性原型验证状态机、业务规则、交互或多种 UI 方向；原型必须标明可删除，结束后删除或吸收入正式实现。
- `skills/mattpocock/engineering/to-prd/SKILL.md`：用于把已讨论清楚的需求整理成 PRD；没有 issue tracker 时落到 `.ai/<project>/` 或用户指定路径。
- `skills/mattpocock/engineering/to-issues/SKILL.md`：用于把 PRD、计划或规格拆成可独立领取的纵向切片任务。
- `skills/mattpocock/engineering/triage/SKILL.md`：用于管理 issue 流程、分拣 bug/需求或给 AFK agent 准备任务。
- `skills/mattpocock/productivity/grill-me/SKILL.md`：用于非代码或不需要 `CONTEXT.md`/ADR 副作用的方案压力测试。
- `skills/mattpocock/productivity/handoff/SKILL.md`：用于把当前会话压缩成交接文档，让后续代理接手。
- `skills/mattpocock/productivity/caveman/SKILL.md`：仅当用户明确要求极简、少 token 或 “caveman mode” 时使用。

## 常用触发短语

- “先 zoom out / 看不懂这块 / 给我调用链”：`engineering/zoom-out/SKILL.md`
- “防腐化 / 架构退化 / 耦合太重 / 深模块 / AI 可导航”：`engineering/improve-codebase-architecture/SKILL.md`
- “grill 我 / 压力测试方案 / 统一术语 / ADR / CONTEXT”：`engineering/grill-with-docs/SKILL.md`
- “做个原型 / 试几个方向 / 状态机跑一下 / 让我玩”：`engineering/prototype/SKILL.md`
- “写 PRD / 拆 issue / 切成可领取任务 / triage”：`engineering/to-prd/SKILL.md`、`engineering/to-issues/SKILL.md` 或 `engineering/triage/SKILL.md`
- “交接 / 下个 agent 接着做 / handoff”：`productivity/handoff/SKILL.md`
- “极简 / 少 token / caveman”：`productivity/caveman/SKILL.md`

## 重叠技能：默认不用

- `skills/mattpocock/engineering/tdd/SKILL.md` 与 Superpowers/project TDD 重叠。默认用项目验证闭环和 `skills/superpowers/test-driven-development/SKILL.md`，除非用户明确点名 Matt Pocock `/tdd`。
- `skills/mattpocock/engineering/diagnose/SKILL.md` 与 `skills/debugging.md` 和 Superpowers systematic debugging 重叠。默认用项目 debugging；用户点名 `/diagnose` 时再读 Matt 版本。
- `skills/mattpocock/in-progress/review/SKILL.md` 与 `skills/review.md` 和 Superpowers code review 重叠，且在 `in-progress/` 下。默认不用。
- `skills/mattpocock/productivity/write-a-skill/SKILL.md` 与 `skills/skill-authoring.md`、Codex `skill-creator`、Superpowers `writing-skills` 重叠。MemoChat 项目 skill 优先用 `skills/skill-authoring.md`。
- `skills/mattpocock/engineering/setup-matt-pocock-skills/SKILL.md` 只在 Matt 子 skill 缺少 issue tracker、triage label 或领域文档布局时读取；不要自动跑全局安装流程。

## 显式点名才读取

- `skills/mattpocock/deprecated/**`：已标为 deprecated，除非用户明确点名，否则不要触发。
- `skills/mattpocock/in-progress/**`：实验性内容，除非用户明确点名，否则不要触发。
- `skills/mattpocock/personal/**`：包含上游作者个人路径和习惯，除非用户明确点名且先核对本机路径，否则不要触发。
- `skills/mattpocock/misc/**`：只在用户明确要求对应工具时读取，例如 Claude Code git hooks、pre-commit、shoehorn 或课程练习脚手架。

## 触发例子

- 用户说“看看 ChatServer 有没有架构腐化/怎么防腐化”：读本入口和 `engineering/improve-codebase-architecture/SKILL.md`，不要默认走 Superpowers review。
- 用户说“我看不懂 AIOrchestrator 这一块，先 zoom out”：读本入口和 `engineering/zoom-out/SKILL.md`，先输出模块地图。
- 用户说“把这个方案拆成可领取任务/issue”：读本入口和 `engineering/to-issues/SKILL.md`；没有 issue tracker 时写入 `.ai/<project>/` 或用户指定文件。

## 维护规则

- 保留上游子 skill 原文，项目适配只写在本入口。
- 同步上游时重新 clone `https://github.com/mattpocock/skills`，覆盖 `skills/mattpocock/` 下的上游分类目录，并更新本入口的 commit。
- 同步后运行 `git diff --check -- AGENTS.md skills/mattpocock`，并用 2-3 个模拟请求确认触发不会误读 Superpowers 或 deprecated/personal skill。
