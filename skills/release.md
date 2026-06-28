---
name: memochat-release
description: Use when preparing a MemoChat Git commit, release commit, release note, tag, release candidate, or release validation.
---

# MemoChat 提交与发布

用于普通 commit、release note、tag、release candidate 或正式 release。保持它是提交/发布守门，不替代 `task.md`、`withtest.md` 或 `runtime-smoke.md`。

## Commit 守门

1. 运行 `git status --short`，区分本次改动、用户已有改动、未跟踪文件和 ignored 产物。
2. 只 stage 本次任务相关文件；不要 stage `.ai/`、构建目录、缓存、日志、Docker 数据或用户未授权改动。
3. 检查 staged 范围：`git diff --cached --name-only`、`git diff --cached --stat`、`git diff --cached --check`。
4. 根据改动风险运行最窄有意义验证；最终报告命令和结果，没跑测试就说明原因。

Commit message 使用 Conventional Commits：

```text
<type>(<scope>): <summary>
```

常用 type：`feat`、`fix`、`refactor`、`perf`、`test`、`docs`、`build`、`ci`、`chore`、`style`、`revert`。scope 用真实模块或目录职责，如 `auth`、`chat`、`relation`、`qml`、`server`、`cmake`、`skills`。summary 写清做了什么，不用 `update`、`misc`、`wip` 这类模糊词。

需要说明破坏性变更、issue 或风险时再加 body/footer，例如 `BREAKING CHANGE:`、`Refs:`、`Closes:`。

## Release 守门

1. 确认发布范围：server、client、ops 或 full stack。
2. 确认用户要 commit、tag、release note，还是只要验证。
3. 收集变更：`git log --oneline --decorate -n 50`，有 tag 时用 `<tag>..HEAD`。
4. 按发布范围运行验证；触及部署产物时使用 `linux-full-gcc16`，需要运行时信心时再走 `skills/runtime-smoke.md`。
5. Release note 用简短分组：Added、Changed、Fixed、Ops / Infrastructure；跳过 `.ai/`、缓存、日志和中间实现噪声。

## 禁止

- 不用 `git add .`，除非刚确认工作区只包含本次任务。
- 不把多个无关目标合并成一个 commit；可独立理解和回滚时拆分。
- 不 amend、rebase、reset、checkout 或 clean 用户改动，除非用户明确要求。
- 不提交 secret、真实 token、数据库 dump、模型权重、构建产物或本地配置。
- 只有用户明确批准最终 release 文本时，才创建 release commit 或 tag。
