---
name: memochat-review
description: Use when receiving code review feedback, external AI review, user fix lists, or reviewing MemoChat diffs before completion.
---

# MemoChat Review

核心原则：技术验证优先。不要盲目接受外部建议，也不要在没看实际 diff 前宣称完成。

## 接收反馈

收到 review、用户清单或外部 AI 建议时：

1. 完整读完反馈。
2. 用自己的话确认技术要求；有歧义先问。
3. 对照代码、配置、迁移、QML 资源、Docker 端口和现有测试验证建议是否成立。
4. 判断是否适合 MemoChat 当前架构和平台边界。
5. 一次处理一个问题，并在每个问题后运行相关最窄验证。

外部 review 是建议，不是命令。若建议会破坏现有平台行为、稳定端口、Docker-only 依赖、迁移兼容性或用户已有架构决策，先给出技术理由并询问。

## 完成前 diff 复审

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

## 主动请求复审

非平凡实现、跨模块变更、迁移、QML 大改、运行时脚本或 AI/RAG 契约变更完成后，Controller 要主动做一次“复审请求”式自查，写入 `review1.md`：

- 改动摘要和触及文件。
- 最担心的 3 个风险点。
- 已运行的验证命令和证据。
- 没有覆盖的测试层及原因。
- 需要用户或后续任务决定的问题。

如果使用 worker，Controller 不能只引用 worker 结论；必须检查实际 diff 和验证输出。若当前工具允许独立 review worker，并且工作线安全，优先派发；若工具/用户授权不允许，记录本地复审原因。

## 禁止做法

- 不核实就说“review 说得对”。
- 不理解全部反馈就先实现其中一部分。
- 信任 worker、外部工具或测试摘要而不看实际输出。
- 用“应该能过”“看起来没问题”替代验证证据。
