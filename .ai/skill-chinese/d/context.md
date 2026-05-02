# 上下文

## 任务

用户要求通读 `walkinglabs/learn-harness-engineering`，总结重要 Harness Engineering 要点，并融合到当前 MemoChat skill 中，为 Codex 配置更强的 agent。

## 外部材料

已克隆并阅读：

- `D:\learn-harness-engineering\README.md`
- `D:\learn-harness-engineering\skills\harness-creator\SKILL.md`
- `D:\learn-harness-engineering\skills\harness-creator\references\context-engineering-pattern.md`
- `D:\learn-harness-engineering\skills\harness-creator\references\lifecycle-bootstrap-pattern.md`
- `D:\learn-harness-engineering\skills\harness-creator\references\multi-agent-pattern.md`
- `D:\learn-harness-engineering\skills\harness-creator\references\memory-persistence-pattern.md`
- `D:\learn-harness-engineering\skills\harness-creator\references\gotchas.md`
- 中文讲义：lecture 01、03、07、09、11
- 中文资源模板：`docs\zh\resources\templates\AGENTS.md`

## 提炼要点

- Harness 不是提示词，而是模型外部的工程系统：指令、状态、验证、范围、生命周期。
- 仓库是 agent 唯一稳定事实来源；跨会话需要的信息必须落盘。
- 冷启动测试很重要：新会话只看仓库应能知道系统是什么、怎么跑、当前做到哪、怎样验证。
- WIP=1 是默认安全策略：一次只推进一个原子任务，避免 overreach 和 under-finish。
- 完成定义必须外部化，不能信任 agent 的主观自信。
- 终止校验应分层：构建/静态、行为验证、端到端或运行时确认。
- 失败要归因到 harness 层：任务规范、上下文、环境、状态、验证、范围、可观测性。
- 可观测性有两层：运行时信号和过程信号；两者都应写入任务轨迹。
- 多 agent 并发需要主 agent 综合，worker prompt 自包含，结果异步返回，避免递归 fork 和共享状态竞争。

## 相关本仓文件

- `AGENTS.md`：仓库级 agent 入口，适合加入强制性 harness 规则。
- `skills/SKILL.md`：MemoChat 主任务流水线，适合加入五子系统、冷启动、WIP=1、三层验证闸门。
- `skills/task.md`：常规实现 workflow，适合加入默认 harness 约束。
- `skills/withtest.md`：运行时测试循环，适合强化测试计划、失败归因和可观测证据。
- `skills/observability.md`：适合补充过程可观测性和任务轨迹。
- `skills/planner.md`：适合让自动化任务自带完成定义和验证证据。
- `skills/PROMPTS.md`：委托阶段提示词，适合同步 WIP、冷启动、完成定义和验证日志要求。

## Docker/MCP

本次为文档和 skill 工作流优化，无业务运行时变更，没有执行 Docker/MCP 查询。无需检查固定端口漂移。

## 冷启动答案

- 系统：MemoChat-Qml-Drogon 的 agent/skill 工作流文档体系。
- 当前做到哪：已完成中文化和 ACID/文档同步规则，本轮融合 Harness Engineering 精华。
- 下一步：复查 diff，确认只改文档/`.ai`，记录验证结果。
- 完成证据：外部仓库已克隆读取；目标文档已更新；`git diff -- AGENTS.md skills/...` 复查；`rg` 检查新增 harness 关键词。

## 上下文预算

已加载：外部 README、harness-creator skill、关键 references、5 篇中文讲义和目标 skill 文件。

按需读取：其余课程项目代码和多语言重复内容。

故意未加载：外部仓库全部项目源码、图片、PDF 构建脚本和非中文重复讲义，避免噪音。

## 风险

- 当前工作树已有大量用户/其他 agent 改动，本轮只应触碰文档和 `.ai/skill-chinese/d`。
- `AGENTS.md` 和 `skills/*` 已有未提交中文化改动，diff 会包含早先翻译内容；本轮总结需明确新增的是 harness 强化规则。
