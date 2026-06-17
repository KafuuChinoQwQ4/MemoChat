---
name: memochat-withtest
description: Use when implementing a MemoChat change that needs CI-retained tests or live validation beyond compilation, including unit, functional, stress, boundary, exception, concurrency-safety, data-driven, Docker-backed, login, chat, media, queue, observability, ops UI, or multi-service behavior.
---

# MemoChat 带测试工作流

这个 skill 只管 MemoChat 测试闭环、持久测试位置和 PASS/阻塞判定。通用 TDD 纪律交给 `skills/superpowers/test-driven-development/SKILL.md`，完成证据门交给 `skills/superpowers/verification-before-completion/SKILL.md`。

## 触发

- 变更需要 CI 可重复运行的单元、功能、压力、边界、异常、并发或数据驱动测试。
- 变更需要 Docker-backed、登录、聊天、媒体、队列、观测、ops UI 或多服务运行时验证。
- 只编译不足以证明行为正确。

## 持久测试位置

- C++ GTest：`tests/cpp/<主项目相对路径>/...`，注册进相应 CMake/CTest。
- Python：`tests/python/<主项目相对路径>/...`，优先 `pytest`，也可沿用现有 runner。
- Fixtures：`tests/fixtures/...`
- 负载、smoke 或运行时探针可复用 `tools/`，但不能替代根 `tests/` 下的产品行为测试，除非记录为不可自动化。
- 不要把持久测试放进临时目录、构建目录、`.ai`、日志目录或业务模块本地 `tests`。

## 必须考虑的测试层

按改动风险选择并记录覆盖或不适用原因：

- 红绿测试：新增/修复行为先有最窄失败测试，再实现到通过。
- 单元测试：核心函数、类、解析器、状态机、服务方法或模型更新。
- 功能测试：用户可见正常路径、API/QML 契约、状态变化和持久化结果。
- 压力测试：吞吐、重复请求、批量数据、长连接、队列、缓存或模型加载。
- 边界/异常测试：空输入、非法输入、缺字段、权限失败、依赖不可用、超时、解析/存储失败。
- 并发安全测试：重复点击、并发请求、重连、重复消息、幂等和竞态。
- 数据驱动测试：参数化、fixtures、表格数据或小样本集覆盖 payload/schema/模型字段。
- 冒烟测试：服务启动、核心入口、登录/注册或本次变更最小端到端路径。

## 运行时测试产物

需要运行时循环时，在任务目录维护：

```text
.ai/<project>/<letter>/
  test<N>.md
  result<N>.md
  fix<N>.md
  screenshots/
  logs/
```

最多五轮。相同命令、阶段或环境依赖连续失败/卡住通常 2 次就停止自动尝试，记录阻塞而不是空转。

`test<N>.md` 至少写明：行为、覆盖层、测试文件路径、runner 命令、需要的容器/服务、数据准备/清理、预期日志/数据库/队列/对象存储结果、成功标准。

`result<N>.md` 必须包含：命令、exit code、关键 stdout/stderr、相关日志或 MCP/Docker 查询、新增/更新测试路径、结论。

`fix<N>.md` 只记录并修复该轮发现的窄问题。修复后重新构建/部署，并从受影响的最窄测试开始重跑。

## 判定

- `PASS`：测试证据满足成功标准。
- `TEST_NEEDS_RERUN`：测试设置、时序或断言错误；修测试并重跑。
- `IMPLEMENTATION_NEEDS_FIX`：产品代码或配置错误；修复后重编译、重部署、重跑。
- `BLOCKED_REPEATED_FAILURE`：环境、端口、Docker、WSL 或同一命令重复失败；停止并报告需要用户处理的动作。

不能根据“服务已启动”“脚本看起来正常”推断 PASS。

## 常用命令

```bash
source /root/.memochat-linux-env
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
ctest --preset linux-full-gcc16 --output-on-failure
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

既有探针：

```powershell
tools/scripts/verify/test_register_login.ps1
tools/scripts/verify/test_login.ps1
tools/scripts/verify/test_login2.ps1
tools/scripts/verify/test_login3.ps1
tools/scripts/verify/full_flow_test.ps1
python tools/loadtest/python-loadtest/py_loadtest.py --config tools/loadtest/python-loadtest/config.json --scenario all --total 20 --concurrency 5
```

## 清理

测试数据使用唯一 run id。结束时按计划清理临时数据、临时日志和一次性调试脚本；保留根 `tests/` 下的测试和必要 fixtures。如果启动了本地 MemoChat 服务，按需运行 `tools/scripts/status/stop-all-services.sh`；除非用户要求，不停止 Docker 依赖。
