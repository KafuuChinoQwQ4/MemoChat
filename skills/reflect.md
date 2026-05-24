---
name: memochat-reflect
description: Use when comparing staged AI work with later unstaged user corrections to extract one reusable MemoChat rule.
---

# MemoChat 反思

在代理改动已暂存、用户又进行了额外未暂存修正后使用。

## 输入

`$ARGUMENTS` 可以是 `.ai/<project>` 名称。如果提供，分析 diff 前先读取最新任务上下文。

## 步骤 1：收集证据

运行：

```bash
git status --short
git diff --cached
git diff
```

如果 staged 或 unstaged diff 为空，停止并说明反思需要二者都存在。

如果提供了项目名：

1. 读取 `.ai/<project>/about.md`
2. 找到最新的字母任务目录
3. 读取 `.ai/<project>/<letter>/context.md`
4. 如果存在，读取 `.ai/<project>/<letter>/plan.md`

## 步骤 2：分类修正

询问：

- 用户修复的是任务特定错误，还是可复用项目约定？
- 这个问题是否已经被本地规则、README、脚本或模式覆盖？
- 记录它能否防止未来无关 MemoChat 工作中的错误？
- 它是否涉及 Docker-only 基础设施、稳定端口、`/data` 或 Windows `D:` 存储、CMake presets、运行时脚本、服务边界、迁移，或 QML/server 契约模式？

最多选择一个最强洞察。

## 步骤 3：决定归属位置

- 当用户希望这样做时，将仓库级规则放入本地 agent instruction 文件。
- 将任务特定经验放入 `.ai/<project>/<letter>/review*.md` 或 result log。
- 只有已存在复审清单或用户要求时，才把机械化复审检查放入复审清单。
- 除非用户要求，否则不要创建额外文档文件。

## 步骤 4：编辑前先提议

不要静默编辑 guideline 文件。展示：

- 拟写文本
- 目标文件和位置
- 为什么它足够通用
- 为什么它不重复现有指导

等待用户批准后再应用。

## 规则

- 保持规则简短且正向。
- 不记录一次性 bug 修复。
- 不要把大段 diff 粘进文档。
- 不要与 Docker-only 基础设施或稳定端口要求冲突。
