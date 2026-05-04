# MemoChat

MemoChat 是一个基于 Qt/QML 与 C++ 后端的桌面即时通讯项目。它围绕聊天、联系人、群组、动态、AI 助手、媒体与本地运营工具构建，目标是提供一个可扩展、可观测、便于本地联调的完整 IM 实验平台。

## 项目内容

- **桌面客户端**：使用 Qt/QML 构建，包含登录注册、好友会话、群聊、动态、通话入口、AI 助手和 R18 分区界面。
- **后端服务**：使用 C++、Boost.Asio/Beast、gRPC 等技术实现网关、聊天、状态、验证码、AI 桥接等服务。
- **AI 与 RAG**：通过 AIOrchestrator 连接 Ollama、Qdrant、Neo4j 等组件，支持知识库、记忆和工具调用方向的能力扩展。
- **数据与基础设施**：PostgreSQL、MongoDB、Redis、MinIO、Redpanda、RabbitMQ 等依赖通过 Docker 运行。
- **可观测性**：集成 Prometheus、Grafana、Loki、Tempo、InfluxDB、OpenTelemetry Collector 等本地观测组件。
- **运营工具**：包含 MemoOps 相关客户端和运行时脚本，用于本地服务管理、日志查看和状态辅助。

## 技术栈

- **客户端**：Qt 6 / QML / C++
- **服务端**：C++23 / Boost.Asio / Boost.Beast / gRPC / Protobuf
- **数据层**：PostgreSQL / MongoDB / Redis / MinIO
- **消息与事件**：Redpanda / RabbitMQ
- **AI/RAG**：Ollama / Qdrant / Neo4j / Python 编排服务
- **构建**：CMake / vcpkg / MSVC
- **基础设施**：Docker Compose

## 目录概览

```text
apps/
  client/desktop/      Qt/QML 桌面客户端与共享客户端库
  server/core/         网关、聊天、状态、验证码、AI 等后端服务
infra/
  deploy/              Docker Compose 与部署配置
  Memo_ops/            本地运营工具与运行时目录
tools/
  scripts/             初始化、构建、部署、测试脚本
  loadtest/            本地压测工具
docs/                  架构说明与接口文档
tests/                 单元测试与集成测试
```

## 本地运行

项目主要面向 Windows 本地开发。业务进程在 Windows 上运行，数据库、队列、对象存储、AI/RAG 与可观测性组件通过 Docker 启动。

常用入口：

```powershell
docker compose -f infra\deploy\local\docker-compose.yml up -d
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
tools\scripts\status\deploy_services.bat
```

更完整的架构、端口、服务启动顺序和验证方式请参考 [docs/当前架构基准.md](docs/当前架构基准.md)。

## 当前状态

MemoChat 仍处于持续开发阶段，仓库中同时包含主业务代码、实验性 HTTP/2/HTTP/3 目标、AI/RAG 能力、R18 源插件适配界面和本地运营工具。部分功能用于验证架构和交互流程，接口与实现仍可能继续调整。
