# 验证结果

## 命令

```powershell
rg -n "Harness|harness|WIP=1|冷启动|完成定义|三层|任务轨迹|可观测|事实来源" AGENTS.md skills .ai\skill-chinese\d
```

结果：exit code 0。确认新增 harness 规则已覆盖：

- `AGENTS.md`：事实来源、冷启动、WIP=1、完成定义、三层终止闸门、失败归因、任务轨迹、多 agent 隔离。
- `skills/SKILL.md`：五子系统、冷启动问题、三层完成闸门、上下文预算、完成定义。
- `skills/task.md`：默认 harness 约束、冷启动测试、计划外问题收敛。
- `skills/withtest.md`：单行为测试循环、排除项、可观测信号、失败层归因。
- `skills/observability.md`：运行时信号和过程信号双层记录。
- `skills/planner.md`：自动化任务包含完成定义、验证命令和 WIP 约束。
- `skills/PROMPTS.md`：委托阶段同步 WIP、冷启动、完成定义和验证日志。

```powershell
git diff --stat -- AGENTS.md skills/SKILL.md skills/task.md skills/withtest.md skills/observability.md skills/planner.md skills/PROMPTS.md .ai/skill-chinese
```

结果：exit code 0。显示本次关注的 7 个入口/skill 文件有文档变更。注意这些文件已有前序中文化改动，stat 同时包含早先翻译和本轮 harness 强化。

```powershell
git status --short -- AGENTS.md skills .ai\skill-chinese
```

结果：exit code 0。确认本轮相关范围只有 `AGENTS.md`、`skills/*` 和未跟踪的 `.ai/skill-chinese/`。其中 `skills/ai-rag.md` 等未在本轮编辑的文件仍显示修改，是前序中文化/其他改动遗留，本轮未回退。

## Docker/MCP

未运行 Docker/MCP。原因：本轮只修改 agent/skill 文档和 `.ai` 任务记录，不涉及运行时服务、端口、数据库或配置。

## 文档同步

已同步。由于本轮直接改变 agent/skill 工作流，已更新 `AGENTS.md`、相关 `skills/*.md` 和 `.ai/skill-chinese/d` 记录。

## 剩余风险

- 工作树本来已有大量未提交业务代码和 skill 文件改动；本轮没有回退或整理这些既有改动。
- 外部仓库克隆保存在 `D:\learn-harness-engineering`，未纳入本仓。
