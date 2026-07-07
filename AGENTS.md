# MemoChat 代理说明

## Skill 优先工作流

在处理本项目之前，先读取项目 skills：

1. 始终先读取 `skills/SKILL.md` 和 `skills/rule.md`，了解 MemoChat skill 路由和基本规则。
2. 然后只读取相关的聚焦 skill 文件：
   - `skills/docker-diagnose.md`：用于 Docker、固定端口、健康状态或 MCP 启动问题。
   - `skills/debugging.md`：用于 bug、测试失败、构建失败、异常行为或连续修复无效的问题，先定位根因再修复。
   - `skills/db-migration.md`：用于 Postgres、Redis、MongoDB、MinIO 元数据、Neo4j 或 Qdrant 数据变更。
   - `skills/runtime-smoke.md`：用于部署/启动脚本、服务 smoke 测试、登录/注册/完整流程检查。
   - `skills/observability.md`：用于 Prometheus、Loki、Tempo、Grafana、InfluxDB、cAdvisor、日志、指标和 traces。
   - `skills/ai-rag.md`：用于 AIOrchestrator、Ollama、Qdrant、Neo4j、MCP bridge、知识库和 RAG 工作。
   - `skills/qml-ui.md`：用于 MemoChat QML、MemoOps QML、图标、资源和客户端 UI 检查。
   - `skills/qml-platform-compat.md`：用于 QML 的平台专用兼容路径、WSLg/Linux 透明圆角、Windows/macOS 路径保留或共享 UI 兼容分支。
   - `skills/no-backward-compat.md`：用于删除旧版本/旧客户端/旧数据字段兼容代码（双读、旧字段别名、旧协议 key、历史迁移读），或新增/改字段时决定是否保留回退。MemoChat 不向后兼容（3.0 不兼容 2.0）；注意区分要删的旧兼容与要保留的平台适配/错误兜底/强制更新机制。
   - `skills/task.md`：用于普通实现工作流。
   - `skills/clarify-first.md`：用于任务目标、约束或改法不清楚时，先向用户收集更多信息再执行。
   - `skills/withtest.md`：用于实现加 CI/CD 可保留测试、迭代式运行时测试、单元/功能/压力/边界/异常/并发安全/数据驱动测试。
   - `skills/planner.md`：用于可复用的 `.ai/<name>/prompt.md` 和 `tasks.json` 自动化计划。
   - `skills/parallel-agents.md`：用于实现任务的 Controller/worker 并发评估；只有安全并且并行收益大于协调成本时才派发 worker，本地单人执行要记录原因。
   - `skills/review.md`：用于接收代码审查、外部 AI review、用户反馈清单，或在完成前复审实际 diff。
   - `skills/release.md`：用于 Git commit、release note、tag 或发布验证；检查 staged 范围和提交格式。
   - `skills/superpowers/SKILL.md`：用于需要应用项目内 Superpowers 工作流快照时；先读该入口，再只读取相关的 `skills/superpowers/<name>/SKILL.md` 子 skill，不要全量加载目录。
   - `skills/mattpocock/SKILL.md`：用于需要应用项目内 Matt Pocock skills 快照时，尤其是 Superpowers 没覆盖的架构防腐化、领域语言 grilling、zoom-out 代码地图、原型、PRD/issue 切片或 handoff；先读该入口，再只读取相关子 skill。
   - `skills/awesomedesign/SKILL.md`：用于前端/网页视觉方向需要参考 DESIGN.md 设计语言样本时；先读该入口，再只读取相关站点的 `DESIGN.md`。
   - `skills/reflect.md`：用于从用户纠正中学习。
   - `skills/skill-authoring.md`：用于创建、更新、裁剪或复审本项目 skills 和外部 skill 快照入口。
   - `skills/tree-doc.md`：用于新增、删除、移动、重命名文件或文件夹，或文件职责变化时，实时同步各文件夹根部的 `_TREE.md` 目录树文档。
   - `skills/icon.md`：用于 SVG/图标资产工作。
3. 只有在构造委派式或基于产物的阶段提示词时，才使用 `skills/PROMPTS.md`。

保持 skill 使用有选择性：除非任务确实横跨所有 skill，否则不要加载每一个 skill 文件。对于实现工作，评估并行是否有价值；如果任务保持本地单人执行，要记录为什么没有有用的 worker 工作线。
