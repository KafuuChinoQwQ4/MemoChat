# 开发环境

## Linux/WSL

默认工作目录是 `/root/code/MemoChat`。本地 Linux 环境通常先加载：

```bash
source /root/.memochat-linux-env
```

该环境文件提供 Docker、Qt、vcpkg、编译器和运行时路径。不要用 Windows 侧路径替代 Linux 主路径；Windows/PowerShell 只用于旧版脚本或明确的 Windows 客户端检查。

## C++/Qt 工具链

- 服务端目标使用 `MEMOCHAT_CXX_STANDARD`，当前默认 C++26。
- Qt/QML 客户端和 MemoOps 使用 `MEMOCHAT_QT_CXX_STANDARD`，当前默认 C++23。
- Linux 全量 preset 是 `linux-full-gcc16`，使用 `/usr/bin/g++-16`、Ninja、Qt 6.8.3 和 `/data/vcpkg`。
- C++26 reflection 和 GNU modules 属于实验能力，按 preset/目标开关启用。

## Web 工具链

Web 前端位于 `apps/web/`，使用 Vite、React、TypeScript、Zustand、React Query 和 Vitest。常用命令：

```bash
cd apps/web
npm install --legacy-peer-deps
npm run dev
npm run build
npm run test
```

也可以用统一脚本：

```bash
tools/scripts/status/start-memochat-web.sh --diagnose
tools/scripts/status/start-memochat-web.sh --real
```

## Docker 依赖

所有基础设施依赖必须在 Docker 中运行，不要在 Docker 外直接启动 Postgres、Redis、MongoDB、RabbitMQ、Redpanda、MinIO、Qdrant、Neo4j 或观测组件。默认数据根目录是 `/data/docker-data/memochat`。
