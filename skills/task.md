---
description: 通过上下文、计划、实现、Docker 支撑的验证和复审来完成 MemoChat 功能或修复。
---

# MemoChat 任务

用于 `/root/code/MemoChat-Qml-Drogon-linux` 中的常规实现工作。除非用户明确要求 Windows 工作，否则将 `D:\MemoChat-Qml-Drogon` 视为旧版 Windows 检出目录。

实现类任务默认使用 `parallel-agents.md` 中的 Controller 主导并行工作流。Controller 负责架构、计划、契约、worker 派发、集成和最终验收。上下文和第一版契约清楚后，默认派发安全的 worker 工作线。本地单人执行是例外，仅当当前工具/策略环境禁止 worker、用户明确要求单代理、任务确实很小且没有有用的测试/复审工作线、任务严格顺序执行，或不存在安全拆分方式时才允许；实现前必须在 `plan.md` 中记录准确原因。

## 调用方式

将 `$ARGUMENTS` 视为任务。如果它以现有 `.ai/<name>/about.md` 开头，将第一个 token 视为后续项目名，剩余文本作为新任务。

## 必需工作流

1. 创建 `.ai/<project>/<letter>/`。
2. 将上下文收集到 `context.md`。
3. 编写并评估 `plan.md`。
4. 默认开启并发：
   - 在允许时为互不重叠且有用的工作线启动 worker
   - 保持 Controller 负责契约和最终验收
   - 当 worker 派发被阻塞、不安全或没有价值时，实现前记录本地单人原因
5. 一次实现一个计划阶段。
6. 用最窄的相关构建/测试/运行时命令验证，并按“红绿测试 -> 基本功能测试 -> 冒烟测试 -> 边界/异常测试”的梯度形成闭环。
7. 复审 diff 并修复重要问题。
8. 以简洁状态摘要收尾。

## MemoChat 上下文清单

始终考虑相关层：

- C++ 服务：`apps/server/core`。
- QML 客户端：`apps/client/desktop` 和 `infra/Memo_ops/client`。
- 运行时配置：`apps/server/config`、`infra/Memo_ops/config`、运行时服务目录下的已部署配置。
- 数据库迁移：`apps/server/migrations/postgresql`。
- 本地 Docker：`infra/deploy/local/docker-compose.yml` 以及 `infra/deploy/local/compose` 下的 compose 片段。
- 脚本：`tools/scripts` 和 `tools/scripts/status`。
- 测试/负载工具：`tests`、`apps/server/core/common/*/tests`、`tools/loadtest/python-loadtest`。

## Docker 和 MCP 规则

- 容器是基础设施的事实来源。
- 不要在 Docker 外安装或启动本地 Redis/Postgres/Mongo/RabbitMQ/Redpanda 等服务。
- 除非明确要求，否则不要改变稳定端口映射。
- 使用 `docker ps` 和 MCP 工具检查状态。
- 默认 Linux 环境是 WSL 发行版 `archlinux`，仓库路径是 `/root/code/MemoChat-Qml-Drogon-linux`。从 Windows 调用时使用：

```powershell
wsl -d archlinux -- bash -lc 'cd /root/code/MemoChat-Qml-Drogon-linux && source /root/.memochat-linux-env && <command>'
```

- 直接检查时优先使用 Docker 命令：

```bash
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

将任何影响推理的查询记录到 `context.md` 或验证日志。

## 构建选择

将要在 `archlinux` WSL 中部署或运行时测试的代码变更，统一使用 Linux full preset。`deploy_services.sh` 默认从 `build-linux-full-gcc16/bin` 复制服务和客户端产物。

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

仅测试检查时，从完整构建树运行测试：

```bash
ctest --preset linux-full-gcc16 --output-on-failure
```

Windows 全量检查使用 `msvc2022-full`；Linux 全量检查使用 `linux-full-gcc16`。

## 功能完成后的测试闭环

每次写完功能都必须测，不能只停在“能编译”：

1. 红绿测试：为新增行为或修复点补最窄自动化测试，先看它因目标行为缺失而失败，再实现到通过。
2. 基本功能测试：验证正常路径、状态更新、协议/配置契约和必要的数据落库或 UI 反馈。
3. 冒烟测试：运行本次改动相关的最小服务启动、脚本探针或端到端入口检查。
4. 边界/异常测试：按改动风险覆盖空值、非法值、重复请求、权限、依赖不可用、超时、并发或平台差异。
5. 任一层失败时，回到实现修复，重新编译，并从受影响的最窄测试开始继续重跑相关梯度，直到没有已知失败。
6. 如果环境限制导致某类测试无法执行，记录原因、替代验证和剩余风险；不要把跳过写成通过。
7. 如果同一阶段、同一命令或同一环境依赖连续重复失败/卡住，不要无限循环。通常同类问题重试或修复后仍连续出现 2 次就停止自动尝试，记录卡住位置、最后命令、关键输出、已尝试次数、疑似原因和需要用户处理的动作，并向用户返回。

## 运行时脚本

使用现有脚本，不要发明新的编排：

```bash
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
tools/scripts/status/stop-all-services.sh
```

`.bat`/`.ps1` 脚本仅用于旧版 Windows 运行时/客户端检查。Linux 服务运行后，现有 PowerShell smoke 探针仍可从 Windows 使用。

## 实现规则

- 优先使用现有 helper 和模块边界。
- 保持服务端/客户端协议和配置变更同步。
- 当持久化 schema 变更必需时，添加迁移或初始化变更。
- Linux 生成/下载的大文件放在 `/data`；Arch Docker 绑定数据放在 `/data/docker-data/memochat`；只有操作旧版 Windows/Docker Desktop 数据时才使用 `D:`。
- 不要回退用户工作。
- 避免大范围格式化噪音。

## 完成

报告：

- 修改的文件
- 默认使用的并发工作线，或准确的本地单人/阻塞原因
- 验证命令和结果
- 红绿、基本功能、冒烟、边界/异常测试覆盖情况；以及失败后是否已修复、重编译并重测
- 重复失败或卡死时的阻塞点、最后成功步骤、最后失败命令、尝试次数和用户需要处理的事项
- 执行过的 Docker/MCP 检查
- 已知阻塞点或剩余风险
- 后续任务可用的 `.ai` 项目名
