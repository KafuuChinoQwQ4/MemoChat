---
name: memochat-task-think
description: 编排 MemoChat-Qml-Drogon 的多阶段实现工作流。用于需要结构化收集上下文、制定计划、实现、通过 Docker 检查数据、执行 CMake 构建验证、复审，并在本地 .ai 项目目录下保留持久产物的任务。
---

# MemoChat 任务流水线

使用这个 skill 推进 `/root/code/MemoChat-Qml-Drogon-linux` 中的非平凡改动，同时让主线程保持精简，并留下有用的过程产物。除非用户明确要求 Windows 侧工作，否则将 `D:\MemoChat-Qml-Drogon` 视为旧版 Windows 检出目录。

对于实现类工作，`skills/parallel-agents.md` 是强制默认执行模式。主线程作为 Controller：负责架构、计划、共享契约、worker 派发、集成、复审和最终验收。Controller 收集到足够上下文并冻结第一版共享契约后，必须立即派发安全且互不重叠的 worker 工作线以加快交付。本地单人执行是例外，仅当当前工具/策略环境禁止启动 worker、用户明确要求单代理、任务确实很小且没有有用的测试/复审工作线、任务严格顺序执行，或不存在安全拆分方式时才允许。继续本地单人实现前，必须在 `plan.md` 中记录准确的例外原因。

## 项目规则

- 将 WSL bash 中的 Arch Linux 视为构建、部署、运行时和验证工作的默认 shell。
- 仅在旧版 Windows 客户端检查、Docker Desktop 迁移/备份操作，或用户明确要求 Windows 检出目录工作时使用 Windows PowerShell。
- 所有基础设施依赖都必须放在 Docker 中。用 `docker ps` 检查容器；通过 Docker 或已配置的 MCP 工具检查数据库和队列，不要使用本地主机安装的服务。
- Arch 原生 Docker 是默认 Docker 运行时。加载 `/root/.memochat-linux-env`，该脚本会取消 `DOCKER_HOST` 并使用 `/var/run/docker.sock`。
- 保持容器端口稳定。除非任务明确要求，否则不要修改 compose 端口映射。
- Linux 缓存、模型、包、vcpkg 产物、Qt 产物以及生成的大文件，优先下载到 `/data`。
- 本地 Docker 绑定数据使用 `/data/docker-data/memochat`。`D:` 仅用于旧版 Docker Desktop 备份、Windows 产物或明确的 Windows 侧检查。
- 可用时使用 MCP 工具：
  - Docker 服务：Prometheus、Loki、Tempo、Grafana、MinIO、RabbitMQ、InfluxDB、cAdvisor。
  - 数据存储：按配置使用 MongoDB、Neo4j、Qdrant、Redpanda、Postgres、Redis。
- 绝不 reset 或 revert 用户改动。和脏工作区共存。

## 产物布局

创建或复用：

```text
.ai/<project-name>/
  about.md
  a/
    context.md
    plan.md
    review1.md
    logs/
      phase-<name>.prompt.md
      phase-<name>.progress.md
      phase-<name>.result.md
  b/
    ...
```

- `about.md` 是项目级蓝图，写法应像功能已经存在并正常工作。
- `<letter>/context.md` 是给下游代理使用的自包含任务上下文。
- `<letter>/plan.md` 是可执行计划和状态清单。
- `logs/` 记录提示词、进展、验证输出和阻塞点。

## 阶段 0：准备

1. 用 `date` 记录开始时间。
2. 检测是否为后续任务：
   - 如果请求的第一个 token 匹配 `.ai/<token>/about.md`，使用该项目。
   - 否则创建一个简短的 kebab-case 项目名。
3. 选择下一个任务字母（`a`、`b`、...），创建目录，并将任务请求写入 `logs/phase-setup.result.md`。
4. 在委派前，将任何截图或附件摘要写入 `context.md` 或日志文件。
5. 对代码改动，读取 `skills/parallel-agents.md`，默认开启 Controller 主导的并发流程。只要存在安全契约和所有权拆分，就立即派发 worker。若没有派发 worker，必须在实现前把阻塞点或例外原因记录到 `plan.md`。

## 阶段 1：上下文

规划前先收集上下文。

只读取相关内容，但要足够让一个新代理无需原始聊天也能工作：

- `CMakeLists.txt`、`CMakePresets.json`，以及最近模块的 `CMakeLists.txt`。
- C++ 服务读取 `apps/server/core/*`：
  `GateServer`、`GateServerCore`、`ChatServer`、`StatusServer`、`VarifyServer`、`AIServer`、`AIOrchestrator`、`common`。
- QML/客户端工作读取 `apps/client/desktop/*` 和 `infra/Memo_ops/client/*`。
- 运行时/数据库工作读取 `apps/server/config`、`apps/server/migrations/postgresql`、`infra/deploy/local` 和 `tools/scripts`。
- 现有相似代码路径、测试、脚本和配置。

写入：

- `.ai/<project>/about.md`
- `.ai/<project>/<letter>/context.md`

`context.md` 必须包含任务描述、相关文件、数据/服务依赖、触及的 Docker 容器/端口、使用过的 MCP/数据库查询、构建/测试命令以及未决风险。

## 阶段 2：计划

编写 `.ai/<project>/<letter>/plan.md`，包含：

- 任务摘要
- 实现思路
- 要修改/创建的文件
- 带有具体文件/函数步骤的实现阶段
- 必需的 Docker/MCP 检查
- 构建/测试命令
- 状态清单
- 并发决策：
  - Controller 职责
  - 默认要派发的 worker 工作线
  - 工作线写入所有权
  - 如果 worker 派发被阻塞、不安全或被有意跳过，记录准确原因

每个阶段都要足够小，使一个代理或一次聚焦处理可以完成。

并行工作线有价值时，Controller 必须在任何 worker 编辑文件前冻结第一版共享契约。

## 阶段 3：计划评估

编辑前用实际代码复核计划：

- 文件路径和符号存在
- 数据库/容器假设与 Docker 状态一致
- schema/迁移变更覆盖初始化和运行时影响
- 客户端/服务端契约变更两侧都已覆盖
- 构建/测试命令适合触及区域

更新 `plan.md` 并添加 `Assessed: yes`。

## 阶段 4：实现

按阶段实现。

- 手工编辑使用 `apply_patch`。
- 并行工作中，Controller 专注于编排、契约更新、集成和验收，worker 拥有互不重叠的文件范围。
- 遵循现有代码风格和本地 helper API。
- 数据库、队列、对象存储和观测检查保持在 Docker/MCP 内完成。
- 对 Postgres，记住 Docker 端口是主机 `15432` 到容器 `5432`；容器内命令使用 `5432`。
- 探测本地开发 Redis 容器时使用密码 `123456`。
- 不要编辑无关文件，也不要大范围格式化。
- 在 `plan.md` 中标记已完成阶段。

## 阶段 5：验证

对可部署的 Arch/WSL 运行时验证，统一使用 Linux full preset。`deploy_services.sh` 默认从 `build-linux-full-gcc16/bin` 复制服务和客户端产物。

部署/运行时 smoke 前必须构建：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

单元测试从完整构建树运行：

```bash
ctest --preset linux-full-gcc16 --output-on-failure
```

运行时验证优先使用现有脚本：

- `tools/scripts/status/deploy_services.sh`
- `tools/scripts/status/start-all-services.sh`
- `tools/scripts/status/stop-all-services.sh`
- 从 Windows 运行时的旧版 Windows 探针：`tools/scripts/test_register_login.ps1`、`tools/scripts/test_login.ps1`、`tools/scripts/full_flow_test.ps1`
- `tools/loadtest/python-loadtest` 下的 Python 负载测试

`.bat`/`.ps1` 脚本仍是旧版 Windows 等价流程。如果 Arch 中 Docker 不可用，先启动或修复 Arch Docker daemon，再做运行时 smoke。

## 阶段 6：复审

复审实际 diff。优先级：

1. 正确性和数据一致性
2. 线程/生命周期安全
3. API 和 schema 兼容性
4. Docker/运行时配置一致性
5. 缺失的验证
6. 不必要的改动噪音

写入 `.ai/<project>/<letter>/review1.md`。修复必须处理的问题，最多重复三轮。

对于并行工作，Controller 必须读取每个使用过的 `logs/parallel-*.result.md`，检查实际 diff，确认契约与最终代码一致，然后才能接受任务。

## 阶段 7：完成

最终回复前：

- 将验证结果记录到 `logs/phase-verify.result.md`。
- 记录并发结果：使用的 worker 工作线、本地单人原因，或任何被阻塞/延后的工作线。
- 确认没有意外的 Docker 端口/配置漂移。
- 汇总修改文件、执行命令、阻塞点和剩余风险。
- 报告耗时和用于后续任务的项目名。
