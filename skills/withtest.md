---
name: memochat-withtest
description: Use when implementing a MemoChat change that needs live validation beyond compilation, including local Docker-backed service, login, chat, media, queue, observability, ops UI, or multi-service behavior.
---

# MemoChat 带测试工作流

当变更需要超出编译之外的 live validation 时使用，例如登录、聊天持久化、媒体存储、队列、观测、ops UI 或多服务行为。

## 阶段 1：实现

遵循 `task.md`：

1. 创建 `.ai/<project>/<letter>/`
2. 收集上下文
3. 制定计划
4. 默认根据 `parallel-agents.md` 开启 Controller 主导并行工作线
5. 实现
6. 构建
7. 复审

对于运行时较重的工作，在允许启动 worker 且存在安全不重叠范围时，默认派发 Tests Worker 或 Integration Worker，让其在 Backend/Frontend/Data worker 实现期间准备 smoke 探针。Controller 仍负责最终运行时验收。

## 阶段 2：运行时测试循环

维护：

```text
.ai/<project>/<letter>/
  test1.md
  result1.md
  fix1.md
  screenshots/
  logs/
```

最多重复五轮；如果同一阶段、同一命令或同一环境依赖连续重复失败/卡住，通常连续 2 次就停止自动尝试，不要为了凑满五轮而空转。

每轮都按“红绿测试 -> 基本功能测试 -> 冒烟测试 -> 边界/异常测试”组织验证。编译通过只是进入测试循环的前置条件，不是功能完成标准。任一层发现问题，都必须写入 `result<N>.md`，在 `fix<N>.md` 记录修复，重新编译、重新部署，并重跑受影响的最窄测试及后续相关梯度，直到 `PASS` 或达到明确阻塞。

判定 `PASS` 前必须执行完成前证据门：列出证明该轮通过的命令或查询，实际运行并读取 exit code/stdout/stderr/日志，再把证据写入 `result<N>.md`。不能根据“服务已启动”“脚本看起来正常”推断功能完成。

遇到 WSL 无响应、Docker daemon 卡住、构建命令长期无输出、服务端口一直占用、脚本重复超时等环境级问题时，把结果判定为阻塞而不是继续循环。在 `result<N>.md` 中记录卡住位置、最后命令、最后可见输出、已尝试次数、最后成功步骤、疑似原因和需要用户处理的动作。

### 步骤 A：准备环境

先检查 Docker：

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

使用 `infra/deploy/local` 下的 compose 文件或现有脚本启动缺失依赖。保持发布端口不变。

### 步骤 B：部署并启动服务

使用现有脚本：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

如果端口已被绑定，使用 `tools/scripts/status/stop-all-services.sh` 或报告占用进程。除非测试需要重启某个依赖且用户同意，否则不要停止 Docker 依赖。

### 步骤 C：编写测试计划

编写 `.ai/<project>/<letter>/test<N>.md`，包含：

- 要测试的行为
- 红绿测试、基本功能测试、冒烟测试、边界/异常测试的覆盖点；无法覆盖的层要写明原因
- 所需容器/服务
- 要运行的脚本或命令
- 数据准备和清理
- 预期日志/数据库/队列/对象存储结果
- QML/Ops UI 工作所需截图
- 成功标准

测试数据应使用可识别前缀或唯一 run id，并写明清理策略：

- 用户、会话、消息、对象存储 key、队列消息和向量/图数据如何创建。
- 哪些数据会在测试后删除，哪些会保留用于排查。
- 清理命令或查询。
- 如果不能安全清理，记录影响范围和后续处理方式。

优先使用现有探针：

- 从 Windows 运行的 `tools/scripts/test_register_login.ps1`
- 从 Windows 运行的 `tools/scripts/test_login.ps1`
- 从 Windows 运行的 `tools/scripts/test_login2.ps1`
- 从 Windows 运行的 `tools/scripts/test_login3.ps1`
- 从 Windows 运行的 `tools/scripts/full_flow_test.ps1`
- `python tools/loadtest/python-loadtest/py_loadtest.py --config tools/loadtest/python-loadtest/config.json --scenario all --total 20 --concurrency 5`
- `logs`、`artifacts` 和运行时服务输出文件下的服务日志。

### 步骤 D：运行测试

运行计划中的命令。捕获：

- 命令行
- exit code
- 相关 stdout/stderr
- 有用时的 Docker logs 或 Loki logs
- 数据库/队列/对象存储验证查询
- UI 工作的截图

写入 `.ai/<project>/<letter>/result<N>.md`。

### 步骤 E：评估

选择一个结果：

- `PASS`：行为满足成功标准。
- `TEST_NEEDS_RERUN`：测试设置、时序或断言错误；调整测试并重跑。
- `IMPLEMENTATION_NEEDS_FIX`：产品代码/配置错误；编写 `.ai/<project>/<letter>/fix<N>.md`，只修复该问题，重新构建、重新部署，并从受影响的最窄测试开始重启循环。
- `BLOCKED_REPEATED_FAILURE`：同一命令/阶段/环境依赖连续重复失败或卡住；停止自动循环，记录阻塞点和需要用户处理的事项后返回。

除非用户明确希望保留，否则不要在生产代码中留下仅用于调试的 instrumentation。

## 清理

结束时：

- 如果测试启动了本地 MemoChat 服务，停止这些服务
- 除非用户要求停止 Docker 依赖，否则让它们保持运行
- 记录最终状态和留下的任何有状态数据
- 执行测试计划中的安全清理；不能清理的数据必须带 run id 并记录位置
