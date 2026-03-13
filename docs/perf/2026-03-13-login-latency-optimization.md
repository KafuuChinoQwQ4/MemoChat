# MemoChat Login Latency Optimization Report

Date: 2026-03-13

## Executive Summary

This optimization series reduced the login path from a seconds-level chain to a millisecond-level chain on a single-node local environment.

- User-observed baseline: TCP login could take about `8s`
- Historical measured baseline: `tcp_login` p50 `4282.554 ms`, p95 `4995.211 ms`
- Current hot-path local result after optimization:
  - Gate `user_login_total_ms`: `2-7 ms`
  - Full login chain after restart with cache hit: about `40.2 ms`
  - Chat login auth: about `10-15 ms`
- Current forced-miss local result:
  - Gate `mysql_check_pwd_ms`: `48-56 ms`
  - Full login chain: about `76.959 ms`
  - Full login + cold relation bootstrap: about `182.72 ms`

The remaining bottleneck is no longer the chat login path. The main residual cost is a real Gate-side MySQL password check when login cache misses.

## Problem Statement And Baseline

### Original chain

The original login path was effectively:

`HTTP /user_login -> Gate synchronous route selection -> client TCP connect -> Chat re-check token in Redis -> Chat query MySQL base info -> relation bootstrap`

This shape had two structural problems:

- Too many serial dependencies in the user-visible login path
- Cold-start and slow-node behavior directly surfaced to the client

### Baseline evidence

1. User symptom

- The original report from interactive use was that TCP login took about `8s`

2. Historical load-test baseline

Source: `local-loadtest/reports/e2e_green_20260309/05_tcp_login.json`

| Metric | Value |
|---|---:|
| Total | 6 |
| Success Rate | 100% |
| p50 | 4282.554 ms |
| p95 | 4995.211 ms |
| max | 4995.211 ms |

3. Historical service log evidence

Source: `Memo_ops/runtime/services/GateServer/logs/GateServer.log_2026-03-10.json`

- A historical `/user_login` sample was received at `2026-03-10T09:00:03.132Z`
- The same request completed at `2026-03-10T09:00:07.221Z`
- That single request took about `4.1s`
- The same log also shows synchronous `StatusService.GetChatServer` during login, proving the older path still included RPC route selection in-band

## What Changed

### 1. Single authentication and local ticket verification

The login chain was changed to:

`HTTP login once -> Gate issues short-lived login_ticket -> client connects directly to Chat -> Chat verifies ticket locally`

Why this technology was used:

- A signed short-lived ticket removes repeated external lookups from the hottest part of the chain
- Local verification is deterministic and low-latency
- It avoids Redis token revalidation and MySQL base-info lookup in Chat v3 login

Why it was better than keeping the old model:

- The old model made Chat login depend on Redis and MySQL again after HTTP login already succeeded
- That duplicated work and amplified cold-path latency

### 2. Gate local route selection

Gate now selects Chat nodes from local cluster config rather than making login depend on synchronous Status RPC.

Why this technology was used:

- Static cluster config is already available locally
- For login routing, local data is cheaper and more predictable than in-band RPC

Why it helps:

- It removes one network hop from the critical path
- It avoids queueing or failure propagation from StatusServer during login

### 3. Multi-endpoint fast-fail client dialing

The client now uses candidate Chat endpoints and a delayed backup dial strategy instead of waiting on one slow endpoint.

Why this technology was used:

- TCP connect tail latency is often dominated by one bad address or one slow node
- Parallel or staggered fallback is a standard way to reduce client-visible connection tails

Why it helps:

- It prevents one bad endpoint from consuming the entire login timeout budget

### 4. Async telemetry export

Telemetry export was moved out of the request thread.

Why this technology was used:

- Observability should not block the business path
- The previous synchronous export caused request-thread stalls when the collector was unavailable

Why it helps:

- It removed the random `1-2s` HTTP first-byte spikes caused by blocked export attempts

### 5. Relation bootstrap optimization

Relation bootstrap was moved out of the login hot path and then optimized independently with:

- Redis short-TTL bootstrap cache
- MySQL indexes for friend and friend-apply access patterns
- Batch loading to replace N+1 tag lookups
- Startup warmup for relation queries

Why these technologies were used:

- Cache is the most effective way to collapse repeated identical bootstrap reads
- Indexes reduce cold-query scan cost
- Batch queries remove multiplicative latency from per-row tag lookups
- Warmup reduces one-time driver or statement cold cost

### 6. Gate login cache persistence and precise invalidation

Gate now keeps a longer-lived login profile cache and invalidates it precisely on password reset and profile update.

Why this technology was used:

- The remaining slow point is the Gate password-check miss
- A stable login profile cache is the cheapest safe way to avoid repeated DB lookups for active users

Why it helps:

- It turns “restart then first active-user login” back into a cache hit instead of a DB miss

## Current Local Single-Node Results

### Source summary

The following values come from local measurements taken on 2026-03-13 during the optimization pass. They combine:

- direct scripted login measurements
- service stage metrics in Gate and Chat logs
- relation bootstrap measurements from Chat logs

### Before vs after

| Stage | Old implementation | New implementation | Improvement |
|---|---:|---:|---:|
| TCP login p50 | 4282.554 ms | 40.2 ms hot restart-first-login | 99.06% lower |
| TCP login p95 | 4995.211 ms | 76.959 ms forced miss full login | 98.46% lower |
| Gate login total | about 4100 ms sample | 2-7 ms cache hit | about 99.8% lower |
| Gate password check miss | in-band and hidden inside seconds-level chain | 48-56 ms | isolated and bounded |
| Chat login auth | mixed into old path | 10-15 ms | now independently small |
| Relation bootstrap cold | about 1264-1495 ms earlier cold spikes | 94-106 ms | about 91-93% lower |
| Relation bootstrap hot | not stable in old chain | 1-3 ms | effectively near-zero for repeat reads |

### Current detailed local measurements

#### A. Historical baseline

Source: `local-loadtest/reports/e2e_green_20260309/05_tcp_login.json`

| Metric | Value |
|---|---:|
| p50 | 4282.554 ms |
| p95 | 4995.211 ms |
| avg | 4369.108 ms |

#### B. Current hot-path Gate login

Source: `Memo_ops/runtime/services/GateServer/logs/GateServer.log_2026-03-13.json`

Observed repeated cache-hit samples:

- `mysql_check_pwd_ms = 0`
- `route_select_ms = 0-1`
- `ticket_issue_ms = 1`
- `user_login_total_ms = 2-7`

#### C. Current forced-cache-miss login

Source: local scripted measurement on 2026-03-13 after deleting login cache and relation cache

| Metric | Value |
|---|---:|
| HTTP login total | 63.875 ms |
| Gate `mysql_check_pwd_ms` | 48 ms |
| Gate `user_login_total_ms` | 50 ms |
| TCP connect | 0.882 ms |
| Chat auth | 12.203 ms |
| Full login total | 76.959 ms |
| Relation bootstrap cold | 105.76 ms |
| Full login + cold relation bootstrap | 182.72 ms |

#### D. Restart-first-login with cache preserved

Source: local scripted measurement on 2026-03-13 after one populate login, then service restart

| Metric | Value |
|---|---:|
| HTTP login total | 24.643 ms |
| Gate `mysql_check_pwd_ms` | 0 ms |
| Gate `user_login_total_ms` | 4 ms |
| TCP connect | 1.003 ms |
| Chat auth | 14.554 ms |
| Full login total | 40.2 ms |

#### E. Relation bootstrap

Source: Chat logs and local measurements from 2026-03-13

| Metric | Value |
|---|---:|
| cold fill typical | 94-106 ms |
| hot cache hit | 1-3 ms |

## Improvement Breakdown

### End-to-end login

Using the historical `tcp_login` p50 baseline and current restart-first-login value:

- Baseline p50: `4282.554 ms`
- Current hot login: `40.2 ms`
- Absolute reduction: `4242.354 ms`
- Relative reduction: `99.06%`

Using the historical `tcp_login` p95 baseline and current forced-miss full login value:

- Baseline p95: `4995.211 ms`
- Current forced-miss full login: `76.959 ms`
- Absolute reduction: `4918.252 ms`
- Relative reduction: `98.46%`

### Relation bootstrap

Using the earlier cold spike reference range and current cold bootstrap:

- Earlier cold spike: about `1264-1495 ms`
- Current cold bootstrap: about `94-106 ms`
- Reduction: about `91%+`

## Predicted Server-Side Numbers

These are not production measurements. They are derived estimates based on local measurements plus deployment assumptions.

### Assumptions

- Client to Gate RTT: `20-40 ms`
- Gate to Chat and Gate/Chat to Redis/MySQL in same availability zone: `0.5-2 ms`
- Service CPU efficiency comparable to local test machine
- No cross-region traffic
- Active-user login cache hit ratio: `80-95%`

### Prediction method

Use:

`predicted_total = client_network + gate_processing + tcp_connect + chat_auth + optional_relation_bootstrap`

Where:

- `client_network` is roughly `2 * RTT` for HTTP request/response and one TCP handshake effect already amortized into connect + protocol exchange budget
- `gate_processing` uses local `user_login_total_ms`
- `chat_auth` uses local measured `10-15 ms`
- `optional_relation_bootstrap` uses local hot `1-3 ms` or cold `94-106 ms`

### Predicted hot-path server login

Assumptions:

- Cache hit at Gate
- No cold relation bootstrap in the critical path
- RTT `20-40 ms`

Estimated:

| Component | Estimate |
|---|---:|
| client network overhead | 20-40 ms |
| Gate processing | 4-10 ms |
| TCP connect and protocol overhead | 5-15 ms |
| Chat auth | 10-18 ms |
| Total | about 39-83 ms |

Recommended planning number:

- p50 target: `50-70 ms`
- p95 target: `80-120 ms`

### Predicted cold or forced-miss server login

Assumptions:

- Gate login cache miss
- One real MySQL password check
- Cold relation bootstrap requested after login
- RTT `20-40 ms`

Estimated:

| Component | Estimate |
|---|---:|
| client network overhead | 20-40 ms |
| Gate processing including DB miss | 55-80 ms |
| TCP connect and protocol overhead | 5-15 ms |
| Chat auth | 10-18 ms |
| cold relation bootstrap | 95-120 ms |
| Total | about 185-273 ms |

Recommended planning number:

- p50 target: `190-230 ms`
- p95 target: `250-320 ms`

## Why These Techniques Were Correct

### Signed login ticket

This is the highest-leverage change because it removes repeated remote state checks from the most latency-sensitive moment in the user journey.

### Local route selection

This is correct because route selection for login does not need synchronous RPC freshness when the cluster topology is already configured locally.

### Async observability export

This is correct because observability should never dominate request latency. The old behavior violated that rule.

### Redis bootstrap cache plus DB indexes

This is correct because relation bootstrap is a classic read-heavy fan-out payload. Cache handles repetition; indexes and batching handle cold misses.

### Long-lived login cache with precise invalidation

This is correct because it attacks the only remaining bounded hot-path miss cost while keeping correctness through targeted invalidation.

## Residual Bottlenecks

- A true Gate login cache miss still costs about `50-60 ms` because MySQL password verification remains in-band
- Server predictions are derived from local measurements and not yet validated on a real multi-host deployment
- The optimization set is very strong for login latency, but it does not yet eliminate the DB miss path entirely

## Recommended Next Step

If additional optimization is needed, the next target should be cache-first Gate credential verification or a dedicated low-latency auth mirror so that even a login cache miss avoids a direct MySQL trip in the request path.

## Evidence Paths

- Historical TCP login baseline:
  - `local-loadtest/reports/e2e_green_20260309/05_tcp_login.json`
- Historical old Gate slow request:
  - `Memo_ops/runtime/services/GateServer/logs/GateServer.log_2026-03-10.json`
- Current Gate hot-path metrics:
  - `Memo_ops/runtime/services/GateServer/logs/GateServer.log_2026-03-13.json`
- Current Chat relation bootstrap metrics:
  - `Memo_ops/runtime/services/chatserver1/logs/ChatServer.log_2026-03-13.json`
