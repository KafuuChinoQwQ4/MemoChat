# 计划

## 任务摘要

把 Agent ACID 原则加入项目说明和 skill 工作流。

## 思路

在 `AGENTS.md`、主 skill、常规 task skill 中加入完整规则；在 `PROMPTS.md` 的共享规则中加入简短版本，确保委托 agent 也继承。

## 修改文件

- `AGENTS.md`
- `skills/SKILL.md`
- `skills/task.md`
- `skills/PROMPTS.md`
- `.ai/skill-chinese/about.md`
- `.ai/skill-chinese/b/context.md`
- `.ai/skill-chinese/b/plan.md`
- `.ai/skill-chinese/b/logs/phase-verify.result.md`
- `.ai/skill-chinese/b/review1.md`

## Docker / 数据影响

无。

## 实现阶段

1. 在全局和主工作流文档中加入完整 Agent ACID 原则。
2. 在委托模板共享规则中加入简短提醒。
3. 验证 diff 和 Markdown 空白。

## 验证

```powershell
rg -n "Agent ACID|原子性|一致性|隔离性|持久性" AGENTS.md skills
git diff --check -- AGENTS.md skills .ai/skill-chinese
```

## 状态

- [x] 阶段 1：加入规则
- [x] 阶段 2：验证
- [x] 阶段 3：复查

Phases: 3
Assessed: yes
