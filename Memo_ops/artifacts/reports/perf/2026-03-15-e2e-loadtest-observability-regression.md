# MemoChat 2026-03-15 全链路压测与监控复核报告

## 1. Executive Summary

本轮报告基于 2026-03-15 当天两次修复后复测结果重写，结论已经和最初失败态不同。

当前结论：

- `VarifyServer` 的代码级启动阻塞已修复，本地服务现在可以通过 `start_test_services.bat --no-client --skip-ops` 稳定拉起
- `RabbitMQ 127.0.0.1:5672` 仍然缺失，但 `VarifyServer` 已改成降级启动，不再因为 AMQP 不通直接退出
- `local-loadtest` 的运行时账号污染已部分修复：
  - `reset_password` 不再截断运行时 CSV
  - 登录类场景会在低压下自动剔除明显失效账号
- 修复后 `smoke login` 已恢复到 `100%` 成功，说明“服务起不来”和“压测账号全脏”这两个前置问题已经被清掉
- 但高压 `login` 仍然会把链路打回超时态：
  - `http_login 1000/100` 仍只有 `0.0%-0.4%`
  - `tcp_login 500/50` 仍为 `0.0%`
- 修复后的 `all` 套件已经能稳定推进到 `auth -> login`，不再在 `VarifyServer` 启动阶段中断；当前真正的主瓶颈已经从“起不来”转成“高并发登录承压失败”

## 2. Problem Statement And Baseline

### 2.1 本次验证口径

- 修复后全链路套件：
  - `powershell -ExecutionPolicy Bypass -File local-loadtest/run_suite.ps1 -Config local-loadtest/config.json -AccountsCsv Memo_ops/artifacts/loadtest/runtime/accounts/accounts.local.csv -Scenario all`
- 修复后高压登录复测：
  - `powershell -ExecutionPolicy Bypass -File local-loadtest/run_suite.ps1 -Config local-loadtest/config.json -AccountsCsv Memo_ops/artifacts/loadtest/runtime/accounts/accounts.local.csv -Scenario login`
- 修复后低压登录复测：
  - `powershell -ExecutionPolicy Bypass -File local-loadtest/run_suite.ps1 -Config local-loadtest/config.smoke.json -AccountsCsv Memo_ops/artifacts/loadtest/runtime/accounts/accounts.local.csv -Scenario login`

### 2.2 本轮关键基线

- 修复前失败态：
  - `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_025542/`
  - `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_030216/`
  - `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_030407/`
- 修复后结果：
  - `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055303/`
  - `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055116/`
  - `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_060255/`

## 3. What Changed

本轮实际完成了 3 类工程修复。

### 3.1 启动链路修复

1. 修复 `VarifyServer` 配置加载语法错误
- 文件：`server/VarifyServer/config.js`
- 结果：Node 不再因为 `telemetry_sample_ratio` 表达式错误直接 `SyntaxError`

2. 将 `RabbitMQ` 从 `VarifyServer` 启动硬依赖改为降级依赖
- 文件：`server/VarifyServer/server.js`
- 结果：`5672` 不通时只记 `service.bootstrap.rabbitmq_degraded`，服务继续监听 `50051`

3. 修复 Windows 启停脚本
- 文件：
  - `scripts/windows/start_test_services.bat`
  - `scripts/windows/stop_test_services.bat`
- 结果：
  - 启动前会校验端口是否被残留进程占用
  - 会预检 `VarifyServer` 配置是否可加载
  - 会对 `RabbitMQ/Kafka/Zipkin` 缺失给出明确告警
  - 停止脚本会回收残留的 `node server.js`

### 3.2 压测账号状态修复

1. 修复 `reset_password` 后运行时 CSV 被截断的问题
- 文件：`local-loadtest/http_reset_password_load.py`

2. 修正运行时账号密码读取顺序
- 文件：`local-loadtest/memochat_load_common.py`
- 结果：优先使用当前 `password`，只在旧数据缺字段时回退 `last_password`

3. 为登录类场景增加账号验活和回退逻辑
- 文件：
  - `local-loadtest/memochat_load_common.py`
  - `local-loadtest/http_login_load.py`
  - `local-loadtest/tcp_login_load.py`
- 结果：
  - 低压场景会自动剔除 `error=1009` 的失效账号
  - 高压场景下如果探测本身超时，不再把账号集清空导致套件误中断，而是回退到原运行集继续执行

### 3.3 服务在线状态

本轮复测时已确认以下端口在线：

- `8080`
- `50051`
- `50052`
- `8090-8093`
- `50055-50058`
- `18080`
- `19100`

## 4. Current Local Single-Node Results

### 4.1 修复后全链路套件结果

目录：`Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055303/`

| 场景 | 口径 | 成功率 | P95(ms) | RPS | 主要错误 | 结论 |
| --- | --- | ---: | ---: | ---: | --- | --- |
| `auth.verify_code` | `200 / 40` | `100.0%` | `1288.404` | `35.974` | 无 | 验证码链路稳定 |
| `auth.register` | `60 / 10` | `66.7%` | `11204.178` | `0.948` | `exception_timeout=20` | 仍有明显尾延迟 |
| `auth.reset_password` | `30 / 6` | `20.0%` | `15313.314` | `0.518` | `exception_timeout=24` | 失败占主导 |
| `login.http` | `1000 / 100` | `0.4%` | `5039.305` | `19.872` | `exception_timeout=996` | 高压仍基本崩溃 |

说明：

- 这一轮 `all` 已经不再死在 `VarifyServer` 启动阶段
- 套件真实推进到了 `login.http`
- 当 `login.http` 把链路打回 5 秒级超时后，后续 `tcp_login` 阶段的账号验活也被拖慢；本轮后来已补修工具回退逻辑，避免这种“工具先死”

### 4.2 修复后低压登录结果

目录：`Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055116/`

| 场景 | 口径 | 成功率 | P95(ms) | RPS | 主要错误 |
| --- | --- | ---: | ---: | ---: | --- |
| `login.http` | `6 / 1` | `100.0%` | `1035.249` | `0.980` | 无 |
| `login.tcp` | `6 / 1` | `100.0%` | `1039.948` | `0.972` | 无 |

这说明：

- 启动链路与账号状态问题修复后，低压登录链路已经恢复可用
- 当前瓶颈不在“服务起不来”或“账号全错”，而在高并发承压能力

### 4.3 修复后高压登录结果

目录：`Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_060255/`

| 场景 | 口径 | 成功率 | P95(ms) | RPS | 主要错误 |
| --- | --- | ---: | ---: | ---: | --- |
| `login.http` | `1000 / 100` | `0.0%` | `5052.698` | `19.840` | `exception_timeout=1000` |
| `login.tcp` | `500 / 50` | `0.0%` | `5027.652` | `9.959` | `exception_timeout=500` |

这说明：

- 在高压口径下，`Gate -> Status -> Chat` 登录链路仍然会迅速退化到统一 5 秒超时
- 这已经是业务链路承压问题，不再是启动前置问题

### 4.4 当前监控面结果

来源：

- `http://127.0.0.1:18080/api/overview`
- `http://127.0.0.1:19100/metrics`

当前概览保持不变：

- `Memo_ops` 平台在线
- `Grafana/Loki/Tempo` 仍未健康
- 本轮仍只能依赖 JSON 报告和服务日志做主证据，实时 OTel 可视化仍缺失

## 5. Improvement Breakdown

本节拆成“已经修复的回归”和“尚未修复的瓶颈”。

### 5.1 已经修复的回归

1. 启动失败已修复
- 修复前：`VarifyServer` 因配置语法错误和 RabbitMQ 硬依赖可能直接起不来
- 修复后：服务可稳定拉起并监听 `50051`

2. 低压登录已恢复
- 修复前 `smoke login`：`0.0%`
- 修复后 `smoke login`：`100.0%`

3. 运行时账号污染已明显收敛
- 修复前：`reset -> login` 后容易继续使用旧密码
- 修复后：登录类场景会优先使用当前密码，并自动清洗明显失效账号

### 5.2 尚未修复的瓶颈

1. `register/reset_password` 仍然慢
- `register P95 = 11204.178 ms`
- `reset_password P95 = 15313.314 ms`

2. 高压登录仍然统一超时
- `http_login P95 ≈ 5.0 s`
- `tcp_login P95 ≈ 5.0 s`

3. OTel 监控栈仍未恢复
- 当前无法给出 Tempo/Loki 级别的实时 trace 佐证

## 6. Predicted Server-Side Numbers

以下仍是 `derived estimate`，不是当前实测。

### 6.1 在以下前提同时满足时

- 恢复本机 `RabbitMQ 127.0.0.1:5672`
- 恢复 OTel 栈
- 清掉登录阶段的 5 秒级统一超时根因

### 6.2 预测恢复区间

| 场景 | 预测恢复区间 | 依据 |
| --- | --- | --- |
| `verify_code` | `1.0s - 1.4s` | 当前 `200/40` 已稳定在 `P95 1288.404 ms` |
| `http_login` | `几十毫秒到百毫秒级客户端整体` | `smoke login` 已恢复，说明主链路不是硬失败 |
| `tcp_login` | `40 ms - 100 ms` | 2026-03-13 健康态报告 + 本轮低压恢复结果 |

## 7. Why These Techniques Were Correct

### 7.1 先修启动边界是正确的

- 不修 `VarifyServer` 语法错误和 AMQP 启动硬依赖，就没有后续压测可言

### 7.2 先修压测工件污染是正确的

- 如果账号 CSV 自己会漂移，压测结论会把测试问题误判成业务退化

### 7.3 将“低压恢复”和“高压承压”分开看是正确的

- 现在已经能证明：
  - 低压链路是活的
  - 高压链路仍然差
- 这比之前“全部失败，原因混在一起”更接近真实边界

## 8. Residual Bottlenecks

1. `RabbitMQ` 当前仍缺失
- 背景 worker 和副作用链路仍在持续告警

2. `register/reset_password` 仍是高延迟阶段
- `auth` 段尾延迟高，说明 `verify/register/reset` 主链路本身还有明显瓶颈

3. 高压 `login` 统一 5 秒超时
- 当前这是最主要的业务性能问题
- 后续应优先看 `GateServer /user_login`、`StatusServer CheckPwd`、以及是否存在连接池/线程池耗尽

4. 监控后端仍未恢复
- 没有 `Grafana/Loki/Tempo`，当前只能用 JSON 报告和日志做离线分析

5. 账号验活策略在高压下只能做“保护性回退”，不能当根因修复
- 它能避免套件误中断
- 但不能解决业务链路本身的高压超时

## 9. Evidence Paths

- `server/VarifyServer/config.js`
- `server/VarifyServer/server.js`
- `scripts/windows/start_test_services.bat`
- `scripts/windows/stop_test_services.bat`
- `local-loadtest/memochat_load_common.py`
- `local-loadtest/http_login_load.py`
- `local-loadtest/tcp_login_load.py`
- `local-loadtest/http_reset_password_load.py`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055303/01_http_verify_code.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055303/02_http_register.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055303/03_http_reset_password.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055303/04_http_login.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055116/04_http_login.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_055116/05_tcp_login.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_060255/04_http_login.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_060255/05_tcp_login.json`
- `Memo_ops/artifacts/loadtest/runtime/reports/suite_20260315_060255/suite_summary.json`
