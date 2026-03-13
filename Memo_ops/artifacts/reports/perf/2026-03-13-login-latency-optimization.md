# MemoChat 登录时延优化报告

日期：2026-03-13

## 执行摘要

这轮优化把登录链路从秒级串行慢路径，压到了单机环境下的毫秒级热路径。

- 用户体感基线：TCP 登录曾经大约需要 `8s`
- 历史压测基线：`tcp_login` p50 为 `4282.554 ms`，p95 为 `4995.211 ms`
- 当前优化后的单机热路径结果：
  - Gate `user_login_total_ms`：`2-7 ms`
  - 服务重启后、命中缓存的完整登录链路：约 `40.2 ms`
  - Chat 登录鉴权：约 `10-15 ms`
- 当前强制缓存 miss 的单机结果：
  - Gate `mysql_check_pwd_ms`：`48-56 ms`
  - 完整登录链路：约 `76.959 ms`
  - 完整登录加冷关系 bootstrap：约 `182.72 ms`

现在剩余的主要成本已经不在 Chat 登录热路径，而是 Gate 在登录缓存 miss 时仍然需要执行一次真实的 MySQL 密码校验。

## 问题定义与基线

### 原始链路

原来的登录链路可以概括为：

`HTTP /user_login -> Gate 同步选路 -> 客户端 TCP connect -> Chat 再查 Redis token -> Chat 再查 MySQL base info -> relation bootstrap`

这条链路有两个结构性问题：

- 用户可感知的登录主路径里串了太多外部依赖
- 冷启动、慢节点、下游阻塞会直接放大到客户端登录时延

### 基线证据

1. 用户现象

- 最初交互使用时，TCP 登录大约需要 `8s`

2. 历史压测基线

来源：`Memo_ops/artifacts/loadtest/history/e2e/e2e_green_20260309/05_tcp_login.json`

| 指标 | 数值 |
|---|---:|
| Total | 6 |
| Success Rate | 100% |
| p50 | 4282.554 ms |
| p95 | 4995.211 ms |
| max | 4995.211 ms |

3. 历史服务日志证据

来源：`Memo_ops/artifacts/logs/services/GateServer/GateServer.log_2026-03-10.json`

- 一条历史 `/user_login` 请求在 `2026-03-10T09:00:03.132Z` 收到
- 同一请求在 `2026-03-10T09:00:07.221Z` 完成
- 单次请求耗时约 `4.1s`
- 同一日志还能看到登录过程中存在同步 `StatusService.GetChatServer`，说明旧链路确实把选路 RPC 放在主路径里

## 优化内容

### 1. 单次鉴权 + 本地验票

登录主链路改成：

`HTTP 登录一次 -> Gate 签发短效 login_ticket -> 客户端直连 Chat -> Chat 本地验票`

为什么使用这个技术：

- 短效签名票据可以把最热路径里的重复外部查询拿掉
- 本地验票是确定性的、低时延的
- Chat v3 登录不再需要重新查 Redis token 和 MySQL 基础资料

为什么优于旧方案：

- 旧方案在 HTTP 登录成功后，Chat 还要重复依赖 Redis 和 MySQL
- 这既重复工作，也把冷路径和尾延迟继续暴露给用户

### 2. Gate 本地选路

Gate 现在直接基于本地集群配置选 Chat 节点，不再让登录依赖同步 Status RPC。

为什么使用这个技术：

- 集群拓扑本身已经有本地配置
- 对登录选路这种场景，本地数据比 in-band RPC 更稳定、更便宜

为什么有效：

- 去掉了关键路径上的一次网络 hop
- 避免 StatusServer 排队或波动继续传递到登录时延

### 3. 客户端多端点快速失败

客户端现在拿到多个 Chat 候选端点，采用主端点优先、延迟兜底连接的方式，不再只堵在单个慢地址上。

为什么使用这个技术：

- TCP connect 的尾延迟常常由单个坏地址或单个慢节点主导
- 延迟并发兜底是缩短客户端连接长尾的常见有效手段

为什么有效：

- 避免某一个不可用端点吃掉整个登录超时预算

### 4. telemetry 异步导出

Telemetry 导出从请求线程中移出，改为异步处理。

为什么使用这个技术：

- 观测系统不应该阻塞业务主路径
- 旧实现里 collector 不可用时，会把请求线程一起卡住

为什么有效：

- 解决了随机出现的 `1-2s` HTTP 首字节毛刺

### 5. relation bootstrap 独立优化

关系 bootstrap 从登录热路径中拆出后，又单独做了以下优化：

- Redis 短 TTL bootstrap 缓存
- 面向好友和申请查询的 MySQL 索引
- 批量查询替代 N+1 tag 查询
- 启动阶段预热关系查询

为什么使用这些技术：

- 缓存最适合解决重复读取同一份 bootstrap 数据
- 索引适合缩短冷查询扫描成本
- 批量查询可以消掉逐条查询带来的乘法级延迟
- 预热能降低驱动、连接池、语句首次执行的冷启动成本

### 6. Gate 登录缓存持久化与精确失效

Gate 现在保留更长寿命的登录资料缓存，并在密码重置、资料修改等场景做精确失效。

为什么使用这个技术：

- 剩余的主要慢点已经集中到 Gate 的密码校验 miss
- 对活跃用户，稳定的登录资料缓存是最便宜且安全的减压手段

为什么有效：

- 把“服务重启后的首个活跃用户登录”重新变成 cache hit，而不是 DB miss

## 当前单机结果

### 数据来源说明

下面的数据来自 2026-03-13 当天的本地单机测量，综合了：

- 本地脚本化登录测量
- Gate 和 Chat 服务阶段耗时日志
- Chat 的 relation bootstrap 记录

### 优化前后对比

| 阶段 | 旧实现 | 新实现 | 提升 |
|---|---:|---:|---:|
| TCP 登录 p50 | 4282.554 ms | 40.2 ms（重启后首个热登录） | 99.06% 降低 |
| TCP 登录 p95 | 4995.211 ms | 76.959 ms（强制 miss 完整登录） | 98.46% 降低 |
| Gate 登录总耗时 | 约 4100 ms 样本 | 2-7 ms（cache hit） | 约 99.8% 降低 |
| Gate 密码校验 miss | 旧链路中被秒级慢路径掩盖 | 48-56 ms | 已被隔离并且可控 |
| Chat 登录鉴权 | 混在旧慢路径中 | 10-15 ms | 已独立且足够小 |
| relation bootstrap 冷路径 | 早期冷启动毛刺约 1264-1495 ms | 94-106 ms | 约 91-93% 降低 |
| relation bootstrap 热路径 | 旧链路不稳定 | 1-3 ms | 基本接近零成本 |

### 当前详细单机测量

#### A. 历史基线

来源：`Memo_ops/artifacts/loadtest/history/e2e/e2e_green_20260309/05_tcp_login.json`

| 指标 | 数值 |
|---|---:|
| p50 | 4282.554 ms |
| p95 | 4995.211 ms |
| avg | 4369.108 ms |

#### B. 当前 Gate 热路径登录

来源：`Memo_ops/artifacts/logs/services/GateServer/GateServer.log_2026-03-13.json`

多次 cache hit 样本显示：

- `mysql_check_pwd_ms = 0`
- `route_select_ms = 0-1`
- `ticket_issue_ms = 1`
- `user_login_total_ms = 2-7`

#### C. 当前强制缓存 miss 登录

来源：2026-03-13 本地脚本测量，在删除登录缓存和关系缓存之后采集

| 指标 | 数值 |
|---|---:|
| HTTP 登录总耗时 | 63.875 ms |
| Gate `mysql_check_pwd_ms` | 48 ms |
| Gate `user_login_total_ms` | 50 ms |
| TCP connect | 0.882 ms |
| Chat auth | 12.203 ms |
| 完整登录总耗时 | 76.959 ms |
| relation bootstrap 冷路径 | 105.76 ms |
| 完整登录 + 冷 relation bootstrap | 182.72 ms |

#### D. 保留缓存后的重启首登

来源：2026-03-13 本地脚本测量，先填充缓存，再重启服务

| 指标 | 数值 |
|---|---:|
| HTTP 登录总耗时 | 24.643 ms |
| Gate `mysql_check_pwd_ms` | 0 ms |
| Gate `user_login_total_ms` | 4 ms |
| TCP connect | 1.003 ms |
| Chat auth | 14.554 ms |
| 完整登录总耗时 | 40.2 ms |

#### E. relation bootstrap

来源：2026-03-13 Chat 日志与本地测量

| 指标 | 数值 |
|---|---:|
| 冷填充常见值 | 94-106 ms |
| 热缓存命中 | 1-3 ms |

## 提升拆解

### 端到端登录

以历史 `tcp_login` p50 和当前重启后首个热登录做对比：

- 基线 p50：`4282.554 ms`
- 当前热登录：`40.2 ms`
- 绝对减少：`4242.354 ms`
- 相对降幅：`99.06%`

以历史 `tcp_login` p95 和当前强制 miss 的完整登录做对比：

- 基线 p95：`4995.211 ms`
- 当前强制 miss 完整登录：`76.959 ms`
- 绝对减少：`4918.252 ms`
- 相对降幅：`98.46%`

### relation bootstrap

以早期冷启动毛刺区间和当前冷路径做对比：

- 早期冷路径毛刺：约 `1264-1495 ms`
- 当前冷路径：约 `94-106 ms`
- 降幅：约 `91%+`

## 服务器侧预测数据

以下数据不是线上实测，而是基于本地结果和部署假设推导出的估算值。

### 预测假设

- 客户端到 Gate RTT：`20-40 ms`
- Gate 到 Chat、Gate/Chat 到 Redis/MySQL 位于同可用区：`0.5-2 ms`
- 服务 CPU 效率与本地测试机接近
- 不考虑跨地域流量
- 活跃用户登录缓存命中率：`80-95%`

### 推导方法

使用如下公式结构：

`predicted_total = client_network + gate_processing + tcp_connect + chat_auth + optional_relation_bootstrap`

其中：

- `client_network` 近似表示 HTTP 请求响应和 TCP 建连带来的网络往返成本
- `gate_processing` 使用本地 `user_login_total_ms`
- `chat_auth` 使用本地实测的 `10-15 ms`
- `optional_relation_bootstrap` 取本地热路径 `1-3 ms` 或冷路径 `94-106 ms`

### 预测的服务器热路径登录

假设：

- Gate 命中缓存
- 冷 relation bootstrap 不在关键路径里
- RTT 为 `20-40 ms`

估算：

| 组成项 | 估算值 |
|---|---:|
| 客户端网络开销 | 20-40 ms |
| Gate 处理 | 4-10 ms |
| TCP connect 与协议开销 | 5-15 ms |
| Chat 鉴权 | 10-18 ms |
| 总计 | 约 39-83 ms |

建议规划值：

- p50 目标：`50-70 ms`
- p95 目标：`80-120 ms`

### 预测的服务器冷路径或强制 miss 登录

假设：

- Gate 登录缓存 miss
- 需要一次真实 MySQL 密码校验
- 登录后触发冷 relation bootstrap
- RTT 为 `20-40 ms`

估算：

| 组成项 | 估算值 |
|---|---:|
| 客户端网络开销 | 20-40 ms |
| Gate 处理含 DB miss | 55-80 ms |
| TCP connect 与协议开销 | 5-15 ms |
| Chat 鉴权 | 10-18 ms |
| 冷 relation bootstrap | 95-120 ms |
| 总计 | 约 185-273 ms |

建议规划值：

- p50 目标：`190-230 ms`
- p95 目标：`250-320 ms`

## 为什么这些技术是对的

### 签名 login ticket

这是收益最高的一项，因为它把用户旅程中最敏感的登录瞬间，从“反复查远端状态”改成了“本地快速验票”。

### Gate 本地选路

登录选路并不需要每次都依赖同步 RPC 取最新状态，本地集群配置足以支撑这条路径，因此把它从主链路中拿掉是合理的。

### 观测异步导出

观测系统不应该主导业务请求耗时。旧实现违反了这个原则，所以异步化是必要修正，而不是附加优化。

### Redis bootstrap 缓存 + 数据库索引

relation bootstrap 是典型的读多、扇出大、容易出现冷启动长尾的场景。缓存解决重复读取，索引和批量查询解决冷 miss。

### 长 TTL 登录缓存 + 精确失效

它直接针对当前唯一剩下的热路径 miss 成本，同时通过定点失效保持正确性，是成本最低、收益直接的改法。

## 残留瓶颈

- 真正的 Gate 登录缓存 miss 仍然会带来约 `50-60 ms` 的 MySQL 密码校验成本
- 当前服务器侧数字仍然是基于本地数据推导，还没有多机部署实测
- 这一轮优化已经基本消除了登录秒级慢路径，但还没有彻底消掉 DB miss 本身

## 下一步建议

如果还要继续压缩登录时延，下一步应优先考虑 cache-first 的 Gate 凭证校验，或者单独的低时延鉴权镜像，让登录缓存 miss 也尽量避免直接打到 MySQL 主路径。

## 证据路径

- 历史 TCP 登录基线：
  - `Memo_ops/artifacts/loadtest/history/e2e/e2e_green_20260309/05_tcp_login.json`
- 历史 Gate 慢请求样本：
  - `Memo_ops/artifacts/logs/services/GateServer/GateServer.log_2026-03-10.json`
- 当前 Gate 热路径指标：
  - `Memo_ops/artifacts/logs/services/GateServer/GateServer.log_2026-03-13.json`
- 当前 Chat relation bootstrap 指标：
  - `Memo_ops/artifacts/logs/services/chatserver1/ChatServer.log_2026-03-13.json`
