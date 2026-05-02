# 计划

## 任务

融合 `learn-harness-engineering` 的精华到 MemoChat agent skill 体系。

## 思路

不直接复制外部课程长文，而是将可执行的 harness 机制压缩为 Codex 每次任务会遵守的短规则：事实来源、冷启动、WIP=1、完成定义、三层验证、失败归因、任务轨迹、多 agent 隔离。

## 要修改的文件

- `AGENTS.md`
- `skills/SKILL.md`
- `skills/task.md`
- `skills/withtest.md`
- `skills/observability.md`
- `skills/planner.md`
- `skills/PROMPTS.md`
- `.ai/skill-chinese/about.md`
- `.ai/skill-chinese/d/*`

## 要创建的文件

- `.ai/skill-chinese/d/context.md`
- `.ai/skill-chinese/d/plan.md`
- `.ai/skill-chinese/d/review1.md`
- `.ai/skill-chinese/d/logs/phase-setup.result.md`
- `.ai/skill-chinese/d/logs/phase-verify.result.md`

## Docker / 数据影响

无。纯文档和 skill 工作流优化。

## 文档同步

本任务本身修改 agent/skill 工作流，因此同步更新 `AGENTS.md`、`skills/*.md` 和 `.ai` 记录。

## 实现阶段

1. 阅读 MemoChat 主 skill、task skill、skill-creator 指南和现有 `.ai/skill-chinese` 记录。
2. 克隆并阅读 `learn-harness-engineering`，提炼核心模式。
3. 将核心模式融合到 `AGENTS.md`、主 skill 和常用 focused skill。
4. 写入 `.ai/skill-chinese/d` 上下文、计划、验证和复查记录。
5. 复查 diff 和关键词，确认没有业务代码越界。

## 验证

```powershell
git diff -- AGENTS.md skills/SKILL.md skills/task.md skills/withtest.md skills/observability.md skills/planner.md skills/PROMPTS.md
rg -n "Harness|harness|WIP=1|冷启动|完成定义|三层|任务轨迹|可观测" AGENTS.md skills .ai/skill-chinese/d
git status --short -- AGENTS.md skills .ai/skill-chinese
```

## 完成定义

- 外部仓库关键内容已阅读并摘要。
- MemoChat agent/skill 文档包含可执行 harness 规则。
- `.ai` 记录能让下一轮冷启动理解本轮工作。
- 验证命令结果写入日志。

## 范围排除项

- 不改业务代码。
- 不改 Docker 端口或运行时配置。
- 不添加外部仓库内容到本仓。
- 不提交 git commit。

## 状态

- [x] 阶段 1：读取本地上下文
- [x] 阶段 2：读取外部 harness 仓库
- [x] 阶段 3：设计融合方案
- [x] 阶段 4：更新 skill 和 `.ai` 记录
- [x] 阶段 5：验证和复查

Phases: 5
Assessed: yes
