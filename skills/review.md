---
name: memochat-review
description: Use when receiving code review feedback, external AI review, user fix lists, or reviewing MemoChat diffs before completion.
---

# MemoChat Review

这个 skill 是 MemoChat 的 review 适配层，不重复通用 review 流程。

- 处理 review 反馈时，先读取 `skills/superpowers/receiving-code-review/SKILL.md`。
- 完成前需要独立审查时，读取 `skills/superpowers/requesting-code-review/SKILL.md`。
- Matt Pocock `skills/mattpocock/in-progress/review/SKILL.md` 默认不用，除非用户明确点名。
- 本文件只补充 MemoChat 的平台、Docker、schema、QML 和验证边界。

## 反馈适配

按 Superpowers review 流程理解和验证反馈后，额外检查：

- 建议是否破坏现有平台行为、稳定端口、Docker-only 依赖或迁移兼容性。
- 建议是否忽略 QML 共享路径与平台专用路径边界。
- 建议是否和用户已有架构决策、`.ai/<project>/<letter>/plan.md` 或当前运行时约束冲突。
- 每个被采纳的问题都要运行相关最窄验证；不能只接受 reviewer 结论。

## MemoChat 复审清单

最终回复前至少检查：

- `git status --short`
- 相关 `git diff`
- `.ai/<project>/<letter>/plan.md` 状态
- `.ai/<project>/<letter>/logs/phase-verify.result.md`

复审优先级：

1. 正确性、数据一致性和生命周期安全。
2. API、schema、QML 属性和配置契约是否同步。
3. Docker 端口、依赖和运行时脚本是否漂移。
4. 测试是否覆盖红绿、基本功能、smoke、边界/异常中的相关层。
5. 是否有无关格式化、顺手重构或调试残留。

## 主动复审记录

非平凡实现、跨模块变更、迁移、QML 大改、运行时脚本或 AI/RAG 契约变更完成后，Controller 要主动做一次“复审请求”式自查，写入 `review1.md`：

- 改动摘要和触及文件。
- 最担心的 3 个风险点。
- 已运行的验证命令和证据。
- 没有覆盖的测试层及原因。
- 需要用户或后续任务决定的问题。

如果使用 worker，Controller 不能只引用 worker 结论；必须检查实际 diff 和验证输出。若当前工具允许独立 review worker，并且工作线安全，优先派发；若工具/用户授权不允许，记录本地复审原因。

## 禁止做法

- 信任 worker、外部工具或测试摘要而不看实际输出。
- 用“应该能过”“看起来没问题”替代验证证据。
