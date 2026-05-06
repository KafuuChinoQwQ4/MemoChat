# MemoChat 压测报告

日期：2026-05-05

本次报告把“完整登录”和“登录后的业务请求”拆开测，避免把 TCP 协议能力、登录业务成本、数据库/缓存依赖混在一个数字里看。

## 指标说明

- RPS：每秒完成的请求数。这里等价于 QPS，但更强调“请求完成数”。
- P50：一半请求低于这个延迟。
- P95/P99：95%/99% 请求低于这个延迟，用来看尾延迟。
- 成功：业务返回成功，或者依赖读写命令正常完成。

## 环境

- GateServer 端口：`8080`、`8084`
- ChatServer 端口：`8090`、`8091`、`8092`、`8093`、`8094`、`8097`
- AIOrchestrator：`8096`
- Docker 依赖：Redis `6379`、Postgres `15432`、Mongo `27017`、Qdrant `6333`、RabbitMQ `5672`、Redpanda `19092`
- 本次默认分层套件参数：`Total=100`，`Concurrency=10`

## 测试方法

默认分层套件：

```powershell
python tools\loadtest\python-loadtest\run_layered_suite.py --config tools\loadtest\python-loadtest\config.benchmark.json --cases default --total 100 --concurrency 10 --db-total 100 --db-concurrency 10 --timeout-sec 300
```

新增的 TCP 轻请求专项：

```powershell
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.benchmark.json --scenario tcp-heartbeat --total 1000 --concurrency 10
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.benchmark.json --scenario tcp-relation-bootstrap --total 1000 --concurrency 10
```

注意：`tcp-heartbeat` 和 `tcp-relation-bootstrap` 会先完成 Gate + Chat 登录，然后复用已登录 TCP 连接发业务包；表里的延迟只统计登录后的请求阶段，不把登录准备成本算进去。

## 默认分层结果

| 场景 | RPS | 成功 | P50 ms | P95 ms | P99 ms | 最大值 ms |
|---|---:|---:|---:|---:|---:|---:|
| HTTP 健康检查 | 954.489 | 100/100 | 3.424 | 7.509 | 8.301 | 8.729 |
| HTTP 登录 | 430.674 | 100/100 | 12.266 | 30.212 | 32.479 | 32.982 |
| TCP 完整登录 | 113.109 | 100/100 | 80.977 | 119.904 | 131.159 | 142.636 |
| TCP 心跳（已登录连接，100 次） | 184.709 | 100/100 | 53.666 | 93.011 | 94.528 | 100.384 |
| TCP 关系拉取（已登录连接，100 次） | 173.178 | 100/100 | 2.488 | 516.001 | 525.668 | 552.973 |
| Redis 写入 | 1330.452 | 100/100 | 5.375 | 16.844 | 20.501 | 22.596 |
| Redis 读取 | 1839.327 | 100/100 | 3.531 | 7.799 | 9.105 | 12.762 |
| Postgres 写入（新连接） | 51.509 | 100/100 | 180.175 | 242.911 | 268.453 | 271.101 |
| Postgres 读取（新连接） | 66.985 | 100/100 | 143.829 | 185.129 | 197.363 | 198.327 |
| Postgres 写入（连接池） | 381.486 | 100/100 | 20.53 | 64.473 | 68.549 | 68.682 |
| Postgres 读取（连接池） | 1554.586 | 100/100 | 4.765 | 14.823 | 15.848 | 19.579 |
| Mongo 插入 | 532.874 | 25/25 | 4.366 | 25.093 | 43.606 | 43.606 |
| Mongo 查询 | 610.209 | 25/25 | 3.721 | 24.54 | 25.349 | 25.349 |
| Qdrant 写入 | 340.931 | 100/100 | 24.149 | 53.932 | 61.209 | 63.073 |
| Qdrant 检索 | 588.098 | 100/100 | 11.151 | 47.598 | 52.709 | 54.489 |
| AI 健康检查 | 216.095 | 100/100 | 13.772 | 31.305 | 323.397 | 323.7 |

## TCP 轻请求专项

| 场景 | 参数 | RPS | 成功 | P50 ms | P95 ms | P99 ms | 最大值 ms |
|---|---|---:|---:|---:|---:|---:|---:|
| TCP 心跳（已登录连接） | 1000 次 / 10 并发 | 492.324 | 1000/1000 | 13.417 | 30.488 | 68.414 | 567.469 |
| TCP 关系拉取（已登录连接） | 1000 次 / 10 并发 | 710.133 | 1000/1000 | 5.88 | 18.189 | 51.784 | 618.534 |
| TCP 心跳（已登录连接） | 1000 次 / 50 并发 | 307.767 | 1000/1000 | 31.641 | 1005.316 | 2047.771 | 2203.517 |
| TCP 关系拉取（已登录连接） | 1000 次 / 50 并发 | 334.467 | 1000/1000 | 23.052 | 994.299 | 1808.161 | 2231.591 |

这组数据说明：TCP 长连接业务请求确实比完整登录快，但并发拉高到 50 后尾延迟明显变差，说明 ChatServer 处理线程、Redis 状态写入、客户端压测线程或连接上的排队开始产生影响。

另外，当前心跳不是纯内存 echo。`ChatSessionService::HandleHeartbeat` 会刷新会话状态，并写 Redis 在线状态，所以它本身也不是“工业级 TCP echo”那种极轻场景。

## AI 和 RAG 单项

| 场景 | 参数 | RPS | 成功 | P50 ms | P95 ms | P99 ms | 备注 |
|---|---|---:|---:|---:|---:|---:|---|
| RAG 检索（固定小知识集） | 20 次 / 4 并发 | 22.59 | 20/20 | 155.658 | 203.028 | 224.272 | `recall_at_k=1.0`，`hallucination_rate=0.0` |
| RAG 检索（50 文档干扰集） | 9 次 / 3 并发 | 0.433 | 9/9 | 6884.405 | 7005.050 | 7005.050 | `recall@1/3/5=0.4074`，`recall@10=0.6944`，`hallucination_rate=0.6667` |
| Agent 执行 | 6 次 / 2 并发 | 0.033 | 0/6 | 60016.144 | 60145.443 | 60145.443 | 全部 60 秒超时，需要单独排查 AI agent 链路 |

## 结论

- HTTP 登录约 `430 RPS`，TCP 完整登录约 `113 RPS`。TCP 登录低的本质原因不是 TCP 协议慢，而是完整登录链路更重：Gate 登录、Chat 登录、票据验签、Redis 会话/在线状态、重复登录检查、离线消息副作用都在里面。
- 已登录 TCP 轻请求在 10 并发下明显更高：心跳约 `492 RPS`，关系拉取约 `710 RPS`。这证明“TCP 本身”不是当前最主要瓶颈。
- TCP 轻请求在 50 并发下尾延迟明显恶化，说明需要继续看 ChatServer 队列、Redis 往返、线程调度和压测客户端本身的排队。
- Postgres 新连接读写很慢，连接池版本快很多；业务代码应避免热路径频繁新建连接。
- AI 健康检查 P99 有明显尖刺；Agent 执行目前不可作为吞吐指标，因为请求全部超时。

## 已做优化和测试工具改动

- 去掉了 ChatServer 和 GateServerCore Redis 热路径里的同步成功日志。
- 把 Gate、Chat、Status、AI 的 Postgres 池默认值提升到 `12`。
- 在压测套件里加入 Postgres pooled 读写场景。
- 在压测套件里新增：
  - `tcp-heartbeat`
  - `tcp-relation-bootstrap`
- 压测报告扩展为 `min/avg/p50/p75/p90/p95/p99/max` 和错误桶。

## 产物

- 默认分层 JSON：`infra/Memo_ops/artifacts/loadtest/runtime/reports/layered_suite_20260505_075610.json`
- 默认分层 Markdown：`infra/Memo_ops/artifacts/loadtest/runtime/reports/layered_suite_20260505_075610.md`
- TCP 心跳 1000 次 / 10 并发：`infra/Memo_ops/artifacts/loadtest/runtime/reports/tcp_heartbeat_20260505_075749.json`
- TCP 关系拉取 1000 次 / 10 并发：`infra/Memo_ops/artifacts/loadtest/runtime/reports/tcp_relation_bootstrap_20260505_075748.json`
- TCP 心跳 1000 次 / 50 并发：`infra/Memo_ops/artifacts/loadtest/runtime/reports/tcp_heartbeat_20260505_075720.json`
- TCP 关系拉取 1000 次 / 50 并发：`infra/Memo_ops/artifacts/loadtest/runtime/reports/tcp_relation_bootstrap_20260505_075720.json`
- RAG 检索固定小知识集：`infra/Memo_ops/artifacts/loadtest/runtime/reports/rag_search_20260505_075210.json`
- RAG 检索 50 文档干扰集：`infra/Memo_ops/artifacts/loadtest/runtime/reports/rag_search_20260506_082528.json`
- Agent 执行：`infra/Memo_ops/artifacts/loadtest/runtime/reports/agent_run_20260505_075458.json`
