# 模块地图

| 模块 | 路径 | 主要职责 |
| --- | --- | --- |
| 服务端聚合根 | `apps/server/` | 服务核心、运行配置、数据库迁移、AI 工作区和运维数据空间 |
| 服务端核心 | `apps/server/core/` | C++ 微服务、ChatServer、Gate 拆分服务、AIOrchestrator |
| 服务端配置 | `apps/server/config/` | 服务端运行期配置文件 |
| 数据库迁移 | `apps/server/migrations/` | PostgreSQL 等数据库结构迁移脚本 |
| 桌面客户端 | `apps/client/desktop/MemoChat-qml/` | Qt/QML IM 客户端、Live2D 桌宠、通话和媒体 UI |
| Web 前端 | `apps/web/` | React/Vite 浏览器客户端 |
| 本地基础设施 | `infra/deploy/local/` | Envoy、数据库、队列、对象存储、观测组件 compose |
| MemoOps | `infra/Memo_ops/` | 本地运行时目录、运维/监控 QML 工具和部署配置 |
| 工具脚本 | `tools/` | 服务启动、smoke、负载测试、MCP server、辅助脚本 |
| 测试 | `tests/` | C++ GTest、Python 结构/契约测试、fixtures |
| 构建支持 | `cmake/` | CMake helper、格式化、模块支持、包重定向 |
| 开发文档 | `docs/` | 全项目开发入口、模块地图、跨模块契约和 ADR |
| Agent 工作流 | `skills/` | 项目内 AI agent 操作规则 |
| CI/CD | `.github/` | GitHub Actions 工作流和配套脚本 |
| 根配置 | `CMakeLists.txt`、`CMakePresets.json`、`vcpkg.json`、`pyproject.toml`、`pytest.ini` | 构建、依赖、Python 测试和格式化入口 |

## 归属原则

- 业务实现放 `apps/`。
- 运行时依赖和部署配置放 `infra/`。
- 可重复使用的脚本和检查工具放 `tools/`。
- 持久测试放根目录 `tests/`。
- 文档按“全项目入口放 `docs/`，模块细节放模块内 `docs/`”拆分。
