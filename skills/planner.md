---
description: 在 .ai/<name>/ 下创建包含 prompt.md 和 tasks.json 的可复用 MemoChat 自动化计划。
---

# MemoChat Planner

用它为重复性的项目工作创建可复用自动化包。

可复用计划默认应采用 `parallel-agents.md` 中的 Controller 主导并行模型，除非该自动化有意设计为单步骤。worker 任务前应包含一个 Controller 任务，用于架构、规划、契约、派发、集成和验收。

## 输出

创建：

```text
.ai/<name>/
  prompt.md
  tasks.json
```

使用简短 kebab-case 名称。先检查 `.ai/`，避免冲突。

## prompt.md 必须包含

- 目标和范围
- 相关 MemoChat 模块
- 涉及的 Docker 容器和固定端口
- 用于检查状态的 MCP 工具或 Docker 命令
- 准确构建/测试/运行时命令
- 数据准备和清理规则
- 失败处理，尤其是 `archlinux` WSL 端口冲突、Docker 集成和 Docker 健康状态
- Controller 主导并发模型、工作线所有权和本地单人 fallback 规则
- 如果自动化会提交代码，包含 commit 指引

## tasks.json 格式

```json
{
  "tasks": [
    {
      "id": "short-id",
      "title": "Short title",
      "description": "Concrete work to perform",
      "started": false,
      "completed": false,
      "dependencies": []
    }
  ]
}
```

## MemoChat 默认值

- 仓库根目录：`/root/code/MemoChat-Qml-Drogon-linux`。
- 默认 WSL 发行版名：`archlinux`。
- 旧版 Windows 检出目录：`D:\MemoChat-Qml-Drogon`。
- Linux 下载和大型生成资产：优先使用 `/data`。
- `archlinux` Docker 绑定数据：`/data/docker-data/memochat`。
- Windows Docker Desktop 数据和旧版 Windows 产物仅可在迁移/fallback 工作中使用 `D:`。
- 基础设施：仅 Docker。
- 数据存储：Redis `6379`、Postgres host `15432`、Mongo `27017`、MinIO `9000/9001`。
- 队列：Redpanda `19092/18082`、RabbitMQ `5672/15672`。
- AI/RAG：AI Orchestrator `8096`、Ollama `11434`（仅当启用）、Neo4j `7474/7687`、Qdrant `6333/6334`。
- 入口：Envoy Gateway `80`，TLS/HTTP2/HTTP3 `8443/tcp+udp`。
- 观测：Grafana `3000`、Prometheus `9090`、Alertmanager `9093`、InfluxDB `8086`、Loki `3100`、Tempo `3200`、OTel `4317/4318/9411/9464`、cAdvisor `8088`。
- 默认 Linux 构建/测试 preset：`linux-full-gcc16`，它会把服务端和客户端产物写入 `build-linux-full-gcc16/bin`，供 `tools/scripts/status/deploy_services.sh` 使用。
- Windows 全量 preset：`msvc2022-full`。

## 验证

写入文件后，读回一次并确认：

- 每个任务都可执行
- dependencies 无环
- 命令匹配真实仓库路径
- Docker/MCP 假设明确
- Controller 和 worker dependencies 让并行关系清楚
- `.ai/` 文件没有被包含进任何提交指令
