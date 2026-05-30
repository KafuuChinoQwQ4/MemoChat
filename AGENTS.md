# MemoChat-Qml-Drogon 代理说明

## Skill 优先工作流

在处理本项目之前，先读取项目 skills：

1. 始终先读取 `skills/SKILL.md`和`skills/rule.md`，了解 MemoChat 主任务工作流和基本的规则。
2. 然后只读取相关的聚焦 skill 文件：
   - `skills/docker-diagnose.md`：用于 Docker、固定端口、健康状态或 MCP 启动问题。
   - `skills/debugging.md`：用于 bug、测试失败、构建失败、异常行为或连续修复无效的问题，先定位根因再修复。
   - `skills/db-migration.md`：用于 Postgres、Redis、MongoDB、MinIO 元数据、Neo4j 或 Qdrant 数据变更。
   - `skills/runtime-smoke.md`：用于部署/启动脚本、服务 smoke 测试、登录/注册/完整流程检查。
   - `skills/observability.md`：用于 Prometheus、Loki、Tempo、Grafana、InfluxDB、cAdvisor、日志、指标和 traces。
   - `skills/ai-rag.md`：用于 AIOrchestrator、Ollama、Qdrant、Neo4j、MCP bridge、知识库和 RAG 工作。
   - `skills/qml-ui.md`：用于 MemoChat QML、MemoOps QML、图标、资源和客户端 UI 检查。
   - `skills/qml-platform-compat.md`：用于 QML 的平台专用兼容路径、WSLg/Linux 透明圆角、Windows/macOS 路径保留或共享 UI 兼容分支。
   - `skills/task.md`：用于普通实现工作流。
   - `skills/clarify-first.md`：用于任务目标、约束或改法不清楚时，先向用户收集更多信息再执行。
   - `skills/withtest.md`：用于实现加 CI/CD 可保留测试、迭代式运行时测试、单元/功能/压力/边界/异常/并发安全/数据驱动测试。
   - `skills/planner.md`：用于可复用的 `.ai/<name>/prompt.md` 和 `tasks.json` 自动化计划。
   - `skills/parallel-agents.md`：用于每个实现任务的默认 Controller 主导并发工作流，只要安全的并行工作能加速交付就使用。Controller agent 必须负责架构、计划、契约、派发、集成和最终验收；worker agents 负责互不重叠的实现、反馈和运行时工作线。
   - `skills/review.md`：用于接收代码审查、外部 AI review、用户反馈清单，或在完成前复审实际 diff。
   - `skills/reflect.md`：用于从用户纠正中学习。
   - `skills/skill-authoring.md`：用于创建、更新或复审本项目 `skills/*.md`。
   - `skills/release.md`：用于发布准备和验证。
   - `skills/icon.md`：用于 SVG/图标资产工作。
3. 只有在构造委派式或基于产物的阶段提示词时，才使用 `skills/PROMPTS.md`。

保持 skill 使用有选择性：除非任务确实横跨所有 skill，否则不要加载每一个 skill 文件。对于实现工作，始终考虑并行工作流；如果任务保持本地单人执行，要记录为什么没有有用的 worker 工作线。
