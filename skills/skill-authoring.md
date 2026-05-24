---
name: memochat-skill-authoring
description: Use when creating, updating, pruning, or reviewing MemoChat project skill files under skills/.
---

# MemoChat Skill 编写

核心原则：skill 是未来代理的操作约束，不是过程故事。保持短、可触发、可执行、可验证。

## 适用范围

- 新增或修改 `skills/*.md`。
- 调整 `AGENTS.md` 中的 skill 触发说明。
- 从外部流程迁移规则到 MemoChat 项目 skill。
- 复审 skill 是否冗余、过长、不可触发或与项目规则冲突。

## 编写规则

- frontmatter 使用 `name` 和 `description`。
- `name` 使用小写字母、数字和连字符。
- `description` 以 `Use when...` 开头，只描述触发条件，不概述完整流程。
- 正文只放未来任务真正需要的规则；不要写一次性经验叙事。
- 细节过长时放入单独聚焦 skill 或 reference，并说明何时读取。
- 不复制通用常识；只保留 MemoChat 特有约束或容易被代理违反的流程。
- 不与 Docker-only、稳定端口、`archlinux` WSL、`/data`、CMake preset、并行 Controller 或验证闭环规则冲突。

## 修改流程

1. 先读取 `skills/SKILL.md` 和 `skills/rule.md`。
2. 只读取相关聚焦 skill，不全量加载。
3. 写明缺口：当前 skill 缺什么、外部规则哪里更好、为什么适合 MemoChat。
4. 优先改现有 skill；只有触发场景独立且可复用时才新增文件。
5. 更新 `AGENTS.md` 的聚焦 skill 列表。
6. 运行文档级验证：读回改动、检查触发说明、运行 `git diff --check`。
7. 用 2-3 个模拟用户请求做触发检查：写下请求、应读取哪些 skill、为什么不会误读无关 skill。
8. 如果当前工具策略和用户授权允许，针对容易被跳过的纪律性规则派发独立 review/pressure worker；不允许时记录原因。

## 质量检查

- 触发条件是否具体。
- 是否避免了 “TBD/TODO/之后补”。
- 是否给出停止信号或禁止做法，防止代理找借口跳过流程。
- 是否说明了验证命令或验证证据。
- 是否能被未来代理在不读取聊天历史的情况下使用。
- 是否已经在 `AGENTS.md` 或上游 skill 中提供触发入口。
- 是否有模拟请求证明它能被正确触发。
