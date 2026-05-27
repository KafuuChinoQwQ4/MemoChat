---
name: memochat-task-think
description: Use when a non-trivial MemoChat task needs structured context, planning, implementation, Docker-backed checks, CMake validation, review, and persistent .ai task artifacts.
---

# MemoChat 任务流水线

使用这个 skill 推进 `/root/code/MemoChat-Qml-Drogon-linux` 中的非平凡改动，同时让主线程保持精简，并留下有用的过程产物。除非用户明确要求 Windows 侧工作，否则将 `D:\MemoChat-Qml-Drogon` 视为旧版 Windows 检出目录。

对于实现类工作，`skills/parallel-agents.md` 是并发与 Controller 责任的权威规则。这里保留总入口和任务主线，具体的 worker 拆分、例外条件、结果文件和验收门都以 `parallel-agents.md` 为准。

## 项目规则

- 将 WSL 发行版 `archlinux` 中的 Arch Linux 视为构建、部署、运行时和验证工作的默认 shell。
- 从 Windows/PowerShell 调用 Linux 命令时使用发行版名 `archlinux`，并显式 `cd /root/code/MemoChat-Qml-Drogon-linux`：

```powershell
wsl -d archlinux -- bash -lc 'cd /root/code/MemoChat-Qml-Drogon-linux && source /root/.memochat-linux-env && <command>'
```

- 不要使用 `wsl -d Arch`。当前 Windows 侧仓库可见为 `Y:\root\code\MemoChat-Qml-Drogon-linux`，但 WSL 内部路径始终使用 `/root/code/MemoChat-Qml-Drogon-linux`。
- 仅在旧版 Windows 客户端检查、Docker Desktop 迁移/备份操作，或用户明确要求 Windows 检出目录工作时使用 Windows PowerShell。
- 所有基础设施依赖都必须放在 Docker 中。用 `docker ps` 检查容器；通过 Docker 或已配置的 MCP 工具检查数据库和队列，不要使用本地主机安装的服务。
- Arch 原生 Docker 是默认 Docker 运行时。加载 `/root/.memochat-linux-env`，该脚本会取消 `DOCKER_HOST` 并使用 `/var/run/docker.sock`。
- 保持容器端口稳定。除非任务明确要求，否则不要修改 compose 端口映射。
- Linux 缓存、模型、包、vcpkg 产物、Qt 产物以及生成的大文件，优先下载到 `/data`。
- 本地 Docker 绑定数据使用 `/data/docker-data/memochat`。`D:` 仅用于旧版 Docker Desktop 备份、Windows 产物或明确的 Windows 侧检查。
- 当前基础设施基线包含：Envoy Gateway、Redis、Postgres、MongoDB、RabbitMQ、Redpanda、MinIO、Qdrant、Neo4j、AI Orchestrator、Prometheus、Alertmanager、Grafana、Loki、Tempo、OTel Collector、InfluxDB、cAdvisor。Ollama 仅在容器或本机服务实际存在时检查。
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
6. 对大改、实验性改动或跨模块重构，先检查 `git status --short` 并在 `context.md` 记录已有脏文件。需要隔离时建议用户使用 worktree；不得为了隔离执行 reset、checkout 或 revert 用户改动。

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

当任务涉及新功能、跨服务契约、数据库/RAG schema、QML 大改、运行时部署或稳定端口风险时，在 `context.md` 或 `plan.md` 先写轻量设计段：

- 目标和非目标
- API/QML/schema/config 契约
- 平台、数据和兼容性边界
- 验证策略和回滚/阻塞条件

## 阶段 2：计划

编写 `.ai/<project>/<letter>/plan.md`，包含：

- 任务摘要
- 实现思路
- 轻量设计段，若本任务触发阶段 1 的设计条件
- 要修改/创建的文件
- 带有具体文件/函数步骤的实现阶段
- 必需的 Docker/MCP 检查
- 构建/测试命令
- 状态清单
- 计划质量门：
  - 每个实现阶段必须有明确文件路径、函数/模块范围和验证命令。
  - 不要写 `TBD`、`TODO`、`之后补`、`类似上一步` 或没有执行细节的泛化步骤。
  - 步骤应足够小，便于红绿测试、实现、验证和复审逐步推进。
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
- 搜索计划中的占位符、含糊动词和无法执行的测试描述

更新 `plan.md` 并添加 `Assessed: yes`。

## 阶段 4：实现

按阶段实现。

- 手工编辑使用 `apply_patch`。
- 遇到 bug、测试失败、构建失败、异常行为或连续修复无效时，读取 `skills/debugging.md`，先完成根因调查和单一假设验证，再改代码。
- 并行工作中，Controller 专注于编排、契约更新、集成和验收，worker 拥有互不重叠的文件范围。
- 遵循现有代码风格和本地 helper API。
- 数据库、队列、对象存储和观测检查保持在 Docker/MCP 内完成。
- 对 Postgres，记住 Docker 端口是主机 `15432` 到容器 `5432`；容器内命令使用 `5432`。
- 探测本地开发 Redis 容器时使用密码 `123456`。
- 不要编辑无关文件，也不要大范围格式化。
- 在 `plan.md` 中标记已完成阶段。

## 阶段 5：验证

对可部署的 `archlinux` WSL 运行时验证，统一使用 Linux full preset。`deploy_services.sh` 默认从 `build-linux-full-gcc16/bin` 复制服务和客户端产物。

写完功能后必须执行分层测试闭环，直到没有发现问题才能进入收尾：

1. 先做最普通的红绿测试：新增或更新最窄自动化测试，确认它先因目标行为缺失或 bug 存在而失败，再通过实现使其变绿。
2. 然后做基本功能测试：覆盖本次功能的正常路径、关键状态变化、服务端/客户端契约和持久化结果。
3. 再做冒烟测试：用现有脚本或最小端到端流程确认服务能启动、核心入口可用、依赖容器健康。
4. 最后做边界/异常测试：覆盖空输入、非法输入、重复操作、权限/登录状态、网络或依赖失败、并发或超时等与改动相关的风险。
5. 任一测试、编译、部署或 smoke 发现问题，都必须回到实现阶段修复，重新编译，并从受影响的最窄测试开始继续跑完整相关梯度；不能带着已知失败进入完成阶段。
6. 如果某一层测试确实无法执行，必须在验证日志和最终回复中写明原因、风险和替代验证；不能把“未执行”写成“通过”。
7. 如果同一阶段、同一命令或同一环境依赖连续重复失败/卡住（例如 WSL 无响应、Docker daemon 卡住、构建命令长期无输出、服务端口一直占用），不要无限循环。通常同类问题重试或修复后仍连续出现 2 次就停止自动尝试，记录最后卡住的位置、命令、输出、已尝试次数、疑似原因和需要用户处理的动作，然后返回给用户。

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

`.bat`/`.ps1` 脚本仍是旧版 Windows 等价流程。如果 `archlinux` 中 Docker 不可用，先启动或修复 Arch Docker daemon，再做运行时 smoke。

## 阶段 6：复审

复审实际 diff。优先级：

1. 正确性和数据一致性
2. 线程/生命周期安全
3. API 和 schema 兼容性
4. Docker/运行时配置一致性
5. 缺失的验证
6. 不必要的改动噪音

写入 `.ai/<project>/<letter>/review1.md`。修复必须处理的问题，最多重复三轮。

收到用户、外部 reviewer 或 AI review 的反馈时，读取 `skills/review.md`：先理解和验证反馈，再按技术优先级逐项实现。不要盲目接受，也不要在反馈含糊时先实现一部分。

对于并行工作，Controller 必须读取每个使用过的 `logs/parallel-*.result.md`，检查实际 diff，确认契约与最终代码一致，然后才能接受任务。

## 阶段 7：完成

最终回复前：

- 执行完成前证据门：
  1. Identify：列出能证明完成状态的命令、diff 或检查。
  2. Run/Read：实际执行并读取完整输出、exit code 和失败数量。
  3. Verify：只有证据确认时才能说通过；否则报告真实状态、阻塞点和风险。
- 将验证结果记录到 `logs/phase-verify.result.md`。
- 确认红绿测试、基本功能测试、冒烟测试、边界/异常测试的结果都已记录；如果中途发现问题，确认已经修复、重新编译并重跑相关测试梯度。
- 如果因重复失败或环境卡死提前停止，记录阻塞点、最后成功步骤、最后失败命令、尝试次数和需要用户处理的事项。
- 记录并发结果：使用的 worker 工作线、本地单人原因，或任何被阻塞/延后的工作线。
- 确认没有意外的 Docker 端口/配置漂移。
- 汇总修改文件、执行命令、阻塞点和剩余风险。
- 报告耗时和用于后续任务的项目名。
