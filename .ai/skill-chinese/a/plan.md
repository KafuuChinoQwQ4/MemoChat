# 计划

## 任务摘要

将 `AGENTS.md` 和所有 `skills/*.md` 说明文本改为中文。

## 实现思路

保持路径、命令、端口、front matter key、状态枚举和代码块稳定，只翻译说明性文字。

## 修改文件

- `AGENTS.md`
- `skills/*.md`
- `.ai/skill-chinese/about.md`
- `.ai/skill-chinese/a/context.md`
- `.ai/skill-chinese/a/plan.md`
- `.ai/skill-chinese/a/review1.md`
- `.ai/skill-chinese/a/logs/phase-setup.result.md`
- `.ai/skill-chinese/a/logs/phase-verify.result.md`

## Docker / Data 影响

无。

## 实现阶段

1. 读取主 skill、相关 task skill、全部待翻译文档。
2. 翻译 `AGENTS.md` 与 `skills/*.md`。
3. 搜索残留英文说明并做必要微调。
4. 复查 diff，确认未改动业务代码和运行时配置。

## 验证

```powershell
rg -n "Agent Instructions|Project Rules|Use when|Relevant Areas|Required Workflow" AGENTS.md skills
git diff -- AGENTS.md skills
```

## 状态

- [x] 阶段 1：读取上下文
- [x] 阶段 2：制定计划
- [x] 阶段 3：评估计划
- [x] 阶段 4：翻译文档
- [x] 阶段 5：验证
- [x] 阶段 6：复查

Phases: 6
Assessed: yes
