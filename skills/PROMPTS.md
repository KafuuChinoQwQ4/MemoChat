# MemoChat 阶段提示词

这些模板用于委派代理，或作为同一会话内的检查清单。替换 `<TASK>`、`<PROJECT>`、`<LETTER>` 和 `<REPO_ROOT>`。

## 共享规则

- 在 `/root/code/MemoChat-Qml-Drogon-linux` 中工作。
- 默认 WSL 发行版名是 `archlinux`。从 Windows 调用 Linux 命令时使用 `wsl -d archlinux -- bash -lc 'cd /root/code/MemoChat-Qml-Drogon-linux && source /root/.memochat-linux-env && <command>'`。
- 基础设施依赖必须运行在 Docker 中。Envoy Gateway、Redis、Postgres、MongoDB、Neo4j、Qdrant、Redpanda、RabbitMQ、MinIO、AI Orchestrator、Prometheus、Alertmanager、Loki、Tempo、Grafana、InfluxDB、OTel Collector 和 cAdvisor 都通过 Docker 或 MCP 工具检查。
- 除非任务明确要求，否则不要修改稳定的 Docker 端口。
- Linux 下载、缓存、大型生成文件、vcpkg 产物和 Qt 产物优先使用 `/data`。
- `archlinux` Docker 绑定数据使用 `/data/docker-data/memochat`。`D:` 仅用于 Docker Desktop 迁移备份或明确的旧版 Windows 工作。
- 不要回退用户改动。
- 除非明确要求，否则不要把 `.ai/` 产物纳入提交。
- Linux 示例使用 bash 命令。只有明确的 Windows 侧 smoke 探针或旧版脚本才使用 PowerShell。

## 标准进度契约

对于委派阶段，尽早创建 `.ai/<PROJECT>/<LETTER>/logs/phase-<name>.progress.md`：

```text
心跳: <N>
当前步骤:
已读/已改文件:
发现:
下一检查点:
阻塞: 无
```

保持简短。在自然里程碑处更新。

## 标准回复

回复不超过 8 行：

```text
STATUS: DONE|BLOCKED|APPROVED|NEEDS_CHANGES
ARTIFACTS: <paths>
TOUCHED: <repo paths or none>
BLOCKER: <none or one short line>
```

## 阶段 1：上下文

```text
你正在为 MemoChat-Qml-Drogon 收集上下文。

TASK:
<TASK>

写入：
- .ai/<PROJECT>/about.md
- .ai/<PROJECT>/<LETTER>/context.md

步骤：
1. 阅读仓库结构和相关 README/config 文件。
2. 检查与任务相关的源文件。
3. 当运行时状态相关时，检查 Docker compose/config/migrations/scripts。
4. 只有在有助于验证真实数据库、队列、对象存储或观测状态时，才查询 Docker 或 MCP。
5. 识别相似实现和现有 helper API。

context.md 必须包含：
- 任务重述
- 相关文件以及它们为什么重要
- 涉及的函数/类/config key/schema
- 涉及的 Docker 容器和固定端口
- 执行过的 MCP/Docker 检查
- 推荐的构建/测试/运行时命令
- 风险和未知项

about.md 必须把项目描述成一个已完成的设计，而不是 TODO 列表。
```

## 阶段 2：计划

```text
你正在规划一个 MemoChat 实现任务。

读取：
- .ai/<PROJECT>/<LETTER>/context.md
- context.md 引用的源文件/config 文件

写入 .ai/<PROJECT>/<LETTER>/plan.md，包含：

## 任务
## 方法
## 要修改的文件
## 要创建的文件
## Docker / 数据影响
## 实现阶段
## 验证
## 状态

让每个阶段都具体：准确文件、函数、配置、脚本、迁移和验证命令。
选择能证明变更的最窄构建/测试目标。
```

## 阶段 3：评估计划

```text
你正在评估一个 MemoChat 实现计划。

读取：
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md
- 引用的源文件/config 文件

检查：
1. 路径和符号存在
2. 模块边界正确
3. 客户端/服务端契约同步
4. 数据库迁移/初始化脚本已覆盖
5. Docker 端口和容器假设正确
6. 验证足够且不过度

原地更新 plan.md。
在“状态”下添加 `Phases: <N>`，并在末尾添加 `Assessed: yes`。
```

## 阶段 4：实现

```text
你正在为 MemoChat 实现 Phase <N>。

读取：
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md

只实现这个阶段：
<PHASE_STEPS>

规则：
- 使用现有模式和 helper。
- 保持 Docker/MCP 假设与计划一致。
- 不要编辑无关文件。
- 除计划状态外，不要修改 .ai 文件。
- 完成后在 plan.md 中标记该阶段。
```

## 阶段 5：验证

```text
你正在验证一个 MemoChat 变更。

读取：
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md

选择能证明本次变更的最窄验证。需要刷新可部署 Linux 服务/客户端产物时，从 Linux full 构建输出执行验证；`deploy_services.sh` 从 `build-linux-full-gcc16/bin` 复制产物。

完整构建（适用于会影响可部署 Linux 产物的变更）：
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12

测试（按风险选择最窄相关 preset 或目标；需要全量信心时运行）：
ctest --preset linux-full-gcc16 --output-on-failure

需要时运行 runtime smoke：
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh

需要从 Windows 运行旧版探针时：
tools/scripts/verify/test_register_login.ps1
tools/scripts/verify/test_login.ps1
tools/scripts/verify/full_flow_test.ps1

如果出现端口冲突或 Docker 依赖失败，停止并报告占用进程/容器。
将结果写入 .ai/<PROJECT>/<LETTER>/logs/phase-verify.result.md。
```

## 阶段 6：复审

```text
你正在复审 MemoChat diff。

读取：
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md
- git diff

写入 .ai/<PROJECT>/<LETTER>/review<R>.md。

优先关注：
1. 正确性和持久化一致性
2. 竞态/生命周期/线程安全
3. schema/config/API 兼容性
4. Docker/运行时影响
5. 缺失的验证
6. 不必要的改动噪音

结论必须是 `APPROVED` 或 `NEEDS_CHANGES`。
对于 NEEDS_CHANGES，包含具体到文件级的修复项。
```

## 阶段 7：收尾

```text
在主会话中完成收尾。

检查：
- 计划状态
- 验证日志
- 复审结论
- git status
- 没有意外的 Docker 端口/配置漂移

报告：
- 修改的文件
- 验证命令和结果
- Docker/MCP 检查
- 阻塞点或剩余风险
- 后续任务可用的 .ai 项目名
```
