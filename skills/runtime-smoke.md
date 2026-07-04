---
name: memochat-runtime-smoke
description: Use when validating MemoChat local runtime startup, Docker dependencies, service health, API probes, logs, login/register, full-flow smoke, or cleanup.
---

# MemoChat 运行时 Smoke

用于在代码/配置变更后验证本地 MemoChat 栈是否正常工作。

这个 skill 只管部署、启动、环境探针、服务健康和清理，不负责测试轮次设计。测试轮次、`test<N>.md` / `result<N>.md` / `fix<N>.md` 产物和 PASS 判定归 `skills/withtest.md`。

## 前置条件

- 当 smoke 依赖本仓库 Linux 服务/客户端时，被测试产物应存在于 `build-linux-full-gcc16/bin`；只检查 Docker 依赖、端口或外部健康时不要求本仓库产物。
- 触及会被部署脚本复制的 Linux 服务端/客户端产物时，先运行 `cmake --preset linux-full-gcc16` 和 `cmake --build --preset linux-full-gcc16 --parallel 12`；文档、脚本说明或无需新产物的定向探针可记录跳过原因。
- Docker 依赖运行在 Arch 原生 Docker 下。compose 命令前加载 `/root/.memochat-linux-env`，使 `DOCKER_HOST` 取消设置。
- 没有旧的 Linux 运行时服务进程仍绑定目标端口。
- 从 Windows 调用 Linux smoke/build 时使用发行版名 `archlinux`，并显式进入 Linux 路径：

```powershell
wsl -d archlinux -- bash -lc 'cd /root/code/MemoChat && source /root/.memochat-linux-env && <command>'
```

## Docker 依赖检查

运行：

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

对于媒体或 AI 流程，也检查 MinIO、Qdrant、Neo4j 和 AI Orchestrator；Ollama 只在本地启用时检查。

## 部署和启动

需要刷新可部署产物时使用：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

如果启动报告端口已在监听，运行 `tools/scripts/status/stop-all-services.sh` 或先识别占用进程再重试。除非用户要求，否则不要停止 Docker 依赖。

## Smoke 脚本

选择相关测试：

```powershell
tools/scripts/verify/test_register_login.ps1
tools/scripts/verify/test_login.ps1
tools/scripts/verify/test_login2.ps1
tools/scripts/verify/test_login3.ps1
tools/scripts/verify/full_flow_test.ps1
python tools/loadtest/python-loadtest/py_loadtest.py --config tools/loadtest/python-loadtest/config.json --scenario all --total 20 --concurrency 5
```

这些探针仍以 PowerShell 优先；需要时从 Windows 运行。对于定向 API 检查，在创建新 payload 前优先使用 `tools/scripts` 中现有 JSON payload。

## 日志

只收集相关日志：

- `infra/Memo_ops/artifacts/logs/services` 下的服务 stdout/stderr 文件
- `infra/Memo_ops/runtime/pids` 下的 pid 文件
- `infra/Memo_ops/runtime/artifacts/logs` 下的服务日志
- 如果运行配置指向仓库 `logs/` 和 `artifacts/`，也检查这些目录
- 依赖容器的 Docker logs
- 有用时通过 MCP 查询 Loki

## 评估

标记结果：

- `PASS`：必需的 API/运行时行为正常。
- `ENVIRONMENT_FAIL`：Docker 依赖、端口、凭据或文件锁问题。
- `IMPLEMENTATION_FAIL`：构建后的服务启动了，但行为错误。
- `INCONCLUSIVE`：日志或测试不足以判断。

## 清理

使用：

```bash
tools/scripts/status/stop-all-services.sh
```

除非用户要求停止 Docker 依赖，否则让它们保持运行。
