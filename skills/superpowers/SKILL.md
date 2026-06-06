---
name: memochat-superpowers
description: Use when a MemoChat task should apply the project-local Superpowers workflow snapshot before planning, implementing, debugging, reviewing, or verifying work.
---

# MemoChat Superpowers 本地快照

本目录是从 Codex Superpowers 插件复制出的项目内快照；项目可独立读取，不依赖插件仍然安装。原始复制路径为：

```text
/root/.codex/plugins/cache/openai-curated/superpowers/e2d08a2e/skills
```

## 使用边界

- 先读 `skills/SKILL.md` 和 `skills/rule.md`，再读本入口。
- 不要全量读取 `skills/superpowers/**`；只读取当前任务触发的子 skill。
- 本目录规则不能覆盖 MemoChat 项目规则：Docker-only、稳定端口、默认 `archlinux` WSL、`/data` 缓存、CMake preset、并行 Controller 和验证闭环仍以项目 skills 为准。
- 若项目 skill 与 Superpowers 子 skill 冲突，优先遵守项目 skill 和用户当前指令。
- 子 skill 中提到的 Claude Code `Skill` / `Task` / `TodoWrite` 等工具名，需要按当前 Codex 工具等价执行。

## 子 skill 触发

- `skills/superpowers/using-superpowers/SKILL.md`：用于确认 Superpowers skill 选择规则。
- `skills/superpowers/brainstorming/SKILL.md`：用于新增功能、组件、行为修改或需要先澄清设计的创造性工作。
- `skills/superpowers/writing-plans/SKILL.md`：用于已经有规格或设计，需要写可执行实现计划。
- `skills/superpowers/executing-plans/SKILL.md`：用于执行已有实现计划且不能安全使用子代理时。
- `skills/superpowers/subagent-driven-development/SKILL.md`：用于有独立任务线、可用子代理并需要规格审查和代码质量审查的实现计划。
- `skills/superpowers/dispatching-parallel-agents/SKILL.md`：用于 2 个以上互不重叠、可并行推进的调查或实现任务。
- `skills/superpowers/test-driven-development/SKILL.md`：用于实现功能或修 bug 时需要红绿重构。
- `skills/superpowers/systematic-debugging/SKILL.md`：用于 bug、测试失败、构建失败或异常行为。
- `skills/superpowers/requesting-code-review/SKILL.md`：用于完成任务前请求独立代码审查。
- `skills/superpowers/receiving-code-review/SKILL.md`：用于处理用户、外部 AI 或 reviewer 的代码审查反馈。
- `skills/superpowers/verification-before-completion/SKILL.md`：用于最终声明完成、修复或通过前的证据门。
- `skills/superpowers/using-git-worktrees/SKILL.md`：用于需要隔离大型或高风险改动时。
- `skills/superpowers/finishing-a-development-branch/SKILL.md`：用于实现完成、测试通过后决定合并、PR 或清理。
- `skills/superpowers/writing-skills/SKILL.md`：用于创建或修改 Superpowers 风格 skill；MemoChat 项目 skill 仍优先使用 `skills/skill-authoring.md`。

## MemoChat 高频触发

- 完成前任何“已修复/已通过/已完成”表述：优先读 `verification-before-completion/SKILL.md`。
- bug、测试失败、构建失败、异常行为：通过 `skills/debugging.md` 进入 `systematic-debugging/SKILL.md`。
- test-first、红绿重构、行为回归测试：通过 `skills/withtest.md` 进入 `test-driven-development/SKILL.md`。
- review 反馈：通过 `skills/review.md` 进入 `receiving-code-review/SKILL.md`。
- 完成前需要独立审查：通过 `skills/review.md` 进入 `requesting-code-review/SKILL.md`。
- 高风险或大范围改动需要隔离：读 `using-git-worktrees/SKILL.md`，但不得回退用户改动。
- 有明确多工作线计划时：项目 `skills/parallel-agents.md` 优先，必要时参考 `dispatching-parallel-agents/SKILL.md` 或 `subagent-driven-development/SKILL.md`。

## 维护规则

- 保留子 skill 原文，便于和上游快照比对。
- 需要同步上游时，先重新安装或恢复 Superpowers 插件缓存，再覆盖本目录下对应子目录。
- 同步后运行 `git diff --check`，并确认 `AGENTS.md` 仍能正确引导只读相关子 skill。
