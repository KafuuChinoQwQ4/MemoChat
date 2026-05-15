# MemoChat-Qml-Drogon 代理说明

## UI 编写规则

- 跨平台 UI 工作以兼容性优先。修复 Linux/WSLg、macOS、Windows、Wayland/X11、DPI、字体、合成器或图形后端差异时，要保留已经正常工作的平台注册表象，并通过平台专用 QML 目录、带平台保护的 C++/QML 分支、资源别名或窄范围包装组件来增加兼容性。
- 不要为了单个平台问题重写共享 QML 组件，也不要替换已经正常工作的某个平台视觉路径，除非已经证明该 bug 会影响共享路径。Windows Acrylic/DWM 行为、macOS 行为以及其他已建立的平台路径必须保持不变，除非任务明确要求修改它们。
- 如果必须触碰共享 QML/C++，要保持现有默认视觉和行为兼容，然后为新的平台行为添加可选属性或平台专用包装器。在计划/复审中记录平台边界，并验证被修改的平台，以及任何共享路径被触碰的平台。
- 平台 UI 兼容性优先使用增量结构：`qml/linux`、`qml/windows`、`qml/macos`、`Qt.platform.os` 检查，或 `#ifdef Q_OS_*` 胶水代码。要在 `qml.qrc` 中显式注册平台资源；添加新兼容路径时，不要删除或替换旧的平台文件。
- 对于 WSLg/Linux 的玻璃效果和透明圆角窗口，避免使用未遮罩的 `ShaderEffectSource`/`MultiEffect`、整窗 `layer.*`、`QRegion` mask，以及依赖 `Rectangle.clip` 来裁剪圆角子元素。应使用 Linux 专用的抗锯齿 QML/Shape 外壳，带透明内边距，并且不要有方形背景板。

## 项目规则

- 所有基础设施容器都在 Docker 中。
- Docker 容器必须保持稳定的发布端口。
- 项目工作必须使用 Docker 容器作为数据库、队列、对象存储、可观测性以及 AI/RAG 依赖。
- 修改代码、检查 MCP 或查看数据库时，先查找相关 Docker 容器或已配置的 MCP 工具。
- 默认项目工作现在面向 WSL/Linux，路径为 `/root/code/MemoChat-Qml-Drogon-linux`。
- Linux 依赖、缓存、模型以及大型生成产物优先下载到 `/data`。
- Arch 原生 Docker 是默认运行时。Docker 绑定数据使用 `/data/docker-data/memochat`。
- 仅在 Docker Desktop 迁移备份、旧版 Windows 脚本或明确的 Windows 客户端检查中使用 `D:`。

## Skill 优先工作流

在处理本项目之前，先读取项目 skills：

1. 始终先读取 `skills/SKILL.md`，了解 MemoChat 主任务工作流。
2. 然后只读取相关的聚焦 skill 文件：
   - `skills/docker-diagnose.md`：用于 Docker、固定端口、健康状态或 MCP 启动问题。
   - `skills/db-migration.md`：用于 Postgres、Redis、MongoDB、MinIO 元数据、Neo4j 或 Qdrant 数据变更。
   - `skills/runtime-smoke.md`：用于部署/启动脚本、服务 smoke 测试、登录/注册/完整流程检查。
   - `skills/observability.md`：用于 Prometheus、Loki、Tempo、Grafana、InfluxDB、cAdvisor、日志、指标和 traces。
   - `skills/ai-rag.md`：用于 AIOrchestrator、Ollama、Qdrant、Neo4j、MCP bridge、知识库和 RAG 工作。
   - `skills/qml-ui.md`：用于 MemoChat QML、MemoOps QML、图标、资源和客户端 UI 检查。
   - `skills/task.md`：用于普通实现工作流。
   - `skills/withtest.md`：用于实现加迭代式运行时测试。
   - `skills/planner.md`：用于可复用的 `.ai/<name>/prompt.md` 和 `tasks.json` 自动化计划。
   - `skills/parallel-agents.md`：用于每个实现任务的默认 Controller 主导并发工作流，只要安全的并行工作能加速交付就使用。Controller agent 必须负责架构、计划、契约、派发、集成和最终验收；worker agents 负责互不重叠的实现、反馈和运行时工作线。
   - `skills/reflect.md`：用于从用户纠正中学习。
   - `skills/release.md`：用于发布准备和验证。
   - `skills/icon.md`：用于 SVG/图标资产工作。
3. 只有在构造委派式或基于产物的阶段提示词时，才使用 `skills/PROMPTS.md`。

保持 skill 使用有选择性：除非任务确实横跨所有 skill，否则不要加载每一个 skill 文件。对于实现工作，始终考虑并行工作流；如果任务保持本地单人执行，要记录为什么没有有用的 worker 工作线。
