# MemoChat Kafka + RabbitMQ 协同增强压测报告

## 1. Executive Summary

本次验证覆盖了 `Kafka + RabbitMQ` 协同方案在本地单机服务集群上的关键路径：

- `ChatServer` 私聊/群聊主消息流继续走 `Postgres outbox -> Kafka -> consumer`
- `VarifyServer` 验证码邮件改为 `RabbitMQ` 异步投递
- `GateServer` 密码重置后的登录缓存失效改为 `RabbitMQ` 任务消费
- 关系通知和群/历史相关链路在 Kafka 主流和 RabbitMQ 任务流并存的情况下完成端到端联调

结论：

- 验证码接口在与 2026-03-11 相同的 `100 请求 / 20 并发` 口径下，成功率从 `5%` 提升到 `100%`
- 注册接口在与 2026-03-11 相同的 `20 请求 / 5 并发` 口径下，成功率从 `50%` 提升到 `100%`
- TCP 登录在与 2026-03-13 相同的 `2 请求 / 1 并发` 口径下，平均时延从 `157.627 ms` 降到 `41.614 ms`
- 2026-03-15 早些时候失败的 `group_ops` 与 `history_ack` 场景，在当前代码上已经恢复到 `100%` 成功
- `gate.cache.invalidate` 已完成从“消息能发出但无人消费”到“发布并立即消费”的闭环修复

## 2. Problem Statement And Baseline

本次优化前的主要问题分成两类：

1. 聊天主链路虽已具备 Kafka 基础，但业务与副作用链路未完全闭环，尤其是 `group/history` 在修复前仍存在 `ConnectionResetError` / `RuntimeError`
2. RabbitMQ 补充链路未在全项目闭环：
   - `VarifyServer` 历史上存在验证码高并发超时
   - `GateServer` 的 `reset_pwd -> cache invalidate` 最初只有 publisher 没有 consumer

基线来源说明：

- `verify_code` 与 `register` 的 before 使用 `historical load-test`
- `tcp_login` 的 before 使用 `historical load-test`
- `group_ops` 与 `history_ack` 的 before 使用 `historical load-test`，但属于失败态基线，只能用于功能恢复对比，不能把总耗时直接视为性能基线

### 2.1 Direct Compare Baseline

| 场景 | 基线时间 | 口径 | 成功率 | Avg 延迟(ms) | P95(ms) | RPS | 证据 |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| verify_code | 2026-03-11 | 100 / 20 并发 | 5.0% | 8806.582 | 9048.400 | 2.214 | `Memo_ops/artifacts/loadtest/runtime/reports/http_verify_code_20260311_155233.json` |
| register | 2026-03-11 | 20 / 5 并发 | 50.0% | 12409.065 | 16777.869 | 0.385 | `Memo_ops/artifacts/loadtest/runtime/reports/http_register_20260311_155124.json` |
| tcp_login | 2026-03-13 | 2 / 1 并发 | 100.0% | 157.627 | 199.541 | 6.320 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_login_20260313_163529.json` |
| group_ops | 2026-03-15 | 1 / 1 并发 | 0.0% | 3412.532 | 3412.532 | 0.293 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_group_ops_20260315_064740.json` |
| history_ack | 2026-03-15 | 1 / 1 并发 | 0.0% | 2073.479 | 2073.479 | 0.482 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_history_ack_20260315_064739.json` |

## 3. What Changed

### 3.1 Kafka 侧

- `ChatServer` 主消息流继续固定在 Kafka
- 私聊/群聊持久化成功后发布 `dialog.sync.v1`
- 好友申请 / 通过后发布 `relation.state.v1`
- 仍保留 `Postgres outbox`，避免 DB 与 Kafka 双写窗口

### 3.2 RabbitMQ 侧

- `VarifyServer` 用 RabbitMQ 替代进程内验证码投递队列
- `GateServer` 增加 `gate.cache.invalidate.q` consumer，真正执行登录缓存失效
- `StatusServer` 保留 `status.presence.refresh` 任务消费入口
- `ChatServer` 的 retry/offline/relation/outbox-repair 任务继续走 RabbitMQ

### 3.3 关键运行时证据

- `ChatServer` 运行时显示 `chat.async_event_bus.kafka` 与 `chat.task_bus.rabbitmq` 同时启用  
  证据：`Memo_ops/runtime/artifacts/logs/services/chatserver1/ChatServer.log_2026-03-15.json`
- `VarifyServer` 日志出现 `varify.get_code.queued` 与 `varify.get_code.sent_async`  
  证据：`Memo_ops/artifacts/logs/services/VarifyServer/VarifyServer_20260315.json`
- `GateServer` 日志出现 `gate.side_effect.cache_invalidate_published` 与 `gate.cache_invalidate.processed`  
  证据：`Memo_ops/runtime/artifacts/logs/services/GateServer/GateServer.log_2026-03-15.json`

## 4. Current Local Single-Node Results

### 4.1 Direct Compare Re-run

| 场景 | 当前时间 | 口径 | 成功率 | Avg 延迟(ms) | P95(ms) | RPS | 证据 |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| verify_code | 2026-03-15 | 100 / 20 并发 | 100.0% | 1049.409 | 1146.888 | 18.691 | `Memo_ops/artifacts/loadtest/runtime/reports/http_verify_code_20260315_081346.json` |
| register | 2026-03-15 | 20 / 5 并发 | 100.0% | 2128.173 | 2337.299 | 2.328 | `Memo_ops/artifacts/loadtest/runtime/reports/http_register_20260315_081401.json` |
| tcp_login | 2026-03-15 | 2 / 1 并发 | 100.0% | 41.614 | 59.754 | 23.679 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_login_20260315_081410.json` |
| group_ops | 2026-03-15 | 1 / 1 并发 | 100.0% | 4109.117 | 4109.117 | 0.243 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_group_ops_20260315_081419.json` |
| history_ack | 2026-03-15 | 1 / 1 并发 | 100.0% | 3547.531 | 3547.531 | 0.282 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_history_ack_20260315_081429.json` |

### 4.2 Additional Current Smoke Results

| 场景 | 口径 | 成功率 | Avg 延迟(ms) | P95(ms) | RPS | 证据 |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| verify_code | 4 / 1 并发 | 100.0% | 1024.207 | 1037.167 | 0.976 | `Memo_ops/artifacts/loadtest/runtime/reports/http_verify_code_20260315_081104.json` |
| register | 2 / 1 并发 | 100.0% | 2158.049 | 2233.056 | 0.463 | `Memo_ops/artifacts/loadtest/runtime/reports/http_register_20260315_081114.json` |
| reset_password | 2 / 1 并发 | 100.0% | 2103.286 | 2161.438 | 0.475 | `Memo_ops/artifacts/loadtest/runtime/reports/http_reset_password_20260315_081124.json` |
| tcp_login | 4 / 1 并发 | 100.0% | 40.051 | 48.367 | 24.689 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_login_20260315_081140.json` |
| friendship | 2 / 1 并发 | 100.0% | 2422.793 | 2453.705 | 0.413 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_friendship_20260315_081157.json` |
| group_ops | 2 / 1 并发 | 100.0% | 4306.986 | 4520.863 | 0.232 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_group_ops_20260315_081216.json` |
| history_ack | 2 / 1 并发 | 100.0% | 3682.783 | 3745.451 | 0.271 | `Memo_ops/artifacts/loadtest/runtime/reports/tcp_history_ack_20260315_081229.json` |

## 5. Improvement Breakdown

### 5.1 verify_code

同口径对比：`100 请求 / 20 并发`

| 指标 | Before | After | Delta |
| --- | ---: | ---: | ---: |
| 成功率 | 5.0% | 100.0% | +95.0 pct |
| Avg 延迟 | 8806.582 ms | 1049.409 ms | -7757.173 ms |
| P95 | 9048.400 ms | 1146.888 ms | -7901.512 ms |
| RPS | 2.214 | 18.691 | +16.477 |

说明：

- 这是本次 RabbitMQ 补充式优化收益最明显的场景
- 关键变化不是“SMTP 变快”，而是“SMTP 从同步关键路径移出”
- 历史失败以 `exception_timeout` 为主；当前报告 `top_errors` 为空

### 5.2 register

同口径对比：`20 请求 / 5 并发`

| 指标 | Before | After | Delta |
| --- | ---: | ---: | ---: |
| 成功率 | 50.0% | 100.0% | +50.0 pct |
| Avg 延迟 | 12409.065 ms | 2128.173 ms | -10280.892 ms |
| P95 | 16777.869 ms | 2337.299 ms | -14440.570 ms |
| RPS | 0.385 | 2.328 | +1.943 |

说明：

- 注册前置依赖验证码投递
- 验证码从同步敏感路径移出后，注册成功率和尾延迟同步改善

### 5.3 tcp_login

同口径对比：`2 请求 / 1 并发`

| 指标 | Before | After | Delta |
| --- | ---: | ---: | ---: |
| 成功率 | 100.0% | 100.0% | 0 |
| Avg 延迟 | 157.627 ms | 41.614 ms | -116.013 ms |
| P95 | 199.541 ms | 59.754 ms | -139.787 ms |
| RPS | 6.320 | 23.679 | +17.359 |

说明：

- 登录路径受益于当前更稳定的 Gate/Status/Chat 运行时装配与缓存命中
- 本次结果证明引入 RabbitMQ 任务面没有拖慢 Kafka 主登录相关链路

### 5.4 group_ops 与 history_ack

这两项 before 是失败态基线，因此只能做“功能恢复”结论：

| 场景 | Before | After | 结论 |
| --- | --- | --- | --- |
| group_ops | `0/1` 成功，`RuntimeError` | `1/1` 成功 | Kafka 主链路与群操作副作用链路恢复 |
| history_ack | `0/1` 成功，`ConnectionResetError` | `1/1` 成功 | 历史拉取与 ACK 链路恢复 |

说明：

- 失败态基线提前中断，不能把失败态总耗时与成功态总耗时直接做性能优劣判断
- 当前成功报告中，群消息阶段 `group_message` 为 `245.335 ms`，历史读取与 ACK 的关键阶段都已恢复到几十毫秒级

## 6. Predicted Server-Side Numbers

以下为 `derived estimate`，不是实测。

假设：

- 客户端 RTT：`10~20 ms`
- 同可用区 Gate/Status/Chat/PostgreSQL/Redis/RabbitMQ/Kafka：`1~3 ms`
- Redis 命中率：登录缓存 `>90%`
- 关系初始化不在登录热路径，仍为 `explicit_pull`
- SMTP 供应商耗时不计入 `get_varifycode` 同步响应，只计 RabbitMQ 异步完成时间

公式形状：

- `verify_code_sync = gate_http + gate_to_varify_grpc + redis_write + rabbit_publish + json_encode_decode`
- `tcp_login_total = client_network + gate_processing + status_route + redis_token/cache + chat_ticket_issue`
- `chat_message_persisted = ingress_validate + outbox_tx + kafka_publish + worker_consume + db_persist + receiver_notify`

预测：

| 场景 | 预测服务端响应 |
| --- | --- |
| verify_code 同步受理 | `80~180 ms` |
| tcp_login | `20~60 ms` |
| 私聊 accepted | `20~50 ms` |
| 私聊 persisted 回执 | `80~180 ms` |
| 群聊 persisted 回执 | `120~260 ms` |

解释：

- Kafka 负责把“消息事实落库 + 顺序消费”稳定下来
- RabbitMQ 负责把“邮件、缓存失效、重试、离线通知”从主链路抽离
- 因此生产环境的主要收益应体现在尾延迟更可控、失败重试不阻塞热路径，而不是所有业务总耗时都绝对更低

## 7. Why These Techniques Were Correct

### 7.1 Kafka 适合聊天事实流

- 私聊需要按会话有序
- 群聊需要按 `group_id` 有序
- 消息历史、已读、dialog sync、relation state 都需要可重放和审计能力
- `Postgres outbox + Kafka consumer` 能把“持久化”和“异步派发”拆开，同时保持事实源稳定

### 7.2 RabbitMQ 适合任务流

- 验证码邮件、缓存失效、离线补投、失败重试都属于副作用任务
- 这些任务不要求像聊天主消息那样做顺序重放
- 把这类任务塞回 Kafka 主 consumer 会扩大主链路抖动；用 RabbitMQ 隔离更合理

### 7.3 双总线分工的收益

- Kafka 约束主消息语义
- RabbitMQ 吸收慢副作用和失败重试
- Redis 保留在线态和短 TTL 快速读
- 控制面接口仍同步，不牺牲用户可理解的成功/失败语义

## 8. Residual Bottlenecks

1. `group_ops` 和 `history_ack` 的全流程总耗时仍在秒级，因为压测脚本包含建群、加人、预热发消息、拉历史、ACK 等完整业务编排，不等于单次消息投递时延
2. `verify_code` 同步接口虽已明显改善，但本地仍约 `1.0~1.1 s`，说明除了 SMTP 之外还有应用层等待、gRPC/HTTP 串联或脚本观测口径带来的固定成本
3. `GateServer` 日志中仍有 `http.read.failed end of stream` 噪声，需要单独做 HTTP 客户端断开/长连接处理的日志降噪
4. `status.presence.refresh` 已具备 consumer，但这轮未做单独性能压测
5. 当前结果基于本地单机 Windows 集群，不代表最终跨机房或公网 RTT 下的绝对数字

## 9. Evidence Paths

- 直接对比报告
  - `Memo_ops/artifacts/loadtest/runtime/reports/http_verify_code_20260311_155233.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/http_verify_code_20260315_081346.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/http_register_20260311_155124.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/http_register_20260315_081401.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_login_20260313_163529.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_login_20260315_081410.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_group_ops_20260315_064740.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_group_ops_20260315_081419.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_history_ack_20260315_064739.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_history_ack_20260315_081429.json`
- 当前补充结果
  - `Memo_ops/artifacts/loadtest/runtime/reports/http_reset_password_20260315_081124.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_friendship_20260315_081157.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_group_ops_20260315_081216.json`
  - `Memo_ops/artifacts/loadtest/runtime/reports/tcp_history_ack_20260315_081229.json`
- 运行时日志
  - `Memo_ops/runtime/artifacts/logs/services/chatserver1/ChatServer.log_2026-03-15.json`
  - `Memo_ops/runtime/artifacts/logs/services/GateServer/GateServer.log_2026-03-15.json`
  - `Memo_ops/artifacts/logs/services/VarifyServer/VarifyServer_20260315.json`

