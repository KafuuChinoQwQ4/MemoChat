# 计划

## 任务摘要

加入“项目改动必须同步相关文档”的规则。

## 思路

在仓库级说明和主 workflow 中加入完整规则；在计划、评估、实现、复查和最终报告处增加文档同步检查点；委托模板也加入对应提醒。

## 修改文件

- `AGENTS.md`
- `skills/SKILL.md`
- `skills/task.md`
- `skills/PROMPTS.md`
- `.ai/skill-chinese/about.md`
- `.ai/skill-chinese/c/context.md`
- `.ai/skill-chinese/c/plan.md`
- `.ai/skill-chinese/c/logs/phase-verify.result.md`
- `.ai/skill-chinese/c/review1.md`

## Docker / 数据影响

无。

## 文档同步

本任务本身就是文档规则更新，已同步到仓库级说明、主 skill、常规 task skill 和委托模板。

## 实现阶段

1. 在 `AGENTS.md`、`skills/SKILL.md`、`skills/task.md` 增加文档同步规则。
2. 在 `skills/SKILL.md` 和 `skills/PROMPTS.md` 的计划/评估/实现/复查/报告环节加入文档同步检查点。
3. 验证搜索结果和 Markdown 空白。

## 验证

```powershell
rg -n "文档同步|无需文档更新|相关文档" AGENTS.md skills
git diff --check -- AGENTS.md skills .ai/skill-chinese
```

## 状态

- [x] 阶段 1：加入规则
- [x] 阶段 2：验证
- [x] 阶段 3：复查

Phases: 3
Assessed: yes
