# MemoChat Performance Report

Date: 2026-03-11

## Scope

- Fresh measurements on 2026-03-11:
  - `auth.register`
  - `auth.verify_code`
  - `login.http`
  - `login.tcp`
  - `mysql_capacity`
  - `redis_capacity`
- Historical baseline used for comparison:
  - `http_login_20260303_083642.json`
  - `tcp_login_20260303_082728.json`
  - `tcp_login_20260303_082715.json`

## Executive Summary

- Current application entry chain is unhealthy. `verify_code`, `http_login`, and `tcp_login` all show request timeouts around `8s`, including `http_login` at `1` concurrency.
- Current database and cache synthetic capacity are still available:
  - MySQL: about `391.94 ops/s` at `20` concurrency.
  - Redis: about `569.31 ops/s` at `50` concurrency.
- The current top database-side hotspot is group message write sequencing, not private history read.
- Compared with the historical healthy baseline, the main collapse is in the app/service chain above MySQL and Redis, not in Redis itself and not in private-message read indexing.

## Today Measurements

| Scenario | Config | Success Rate | Throughput | Avg Latency | P95 | Main Errors | Notes |
|---|---:|---:|---:|---:|---:|---|---|
| `auth.register` | `20 total / 5 conc` | `50.0%` | `0.385 rps` | `12409.1 ms` | `16777.9 ms` | `exception_timeout=9`, `register_err_1005=1` | Registered `10` new accounts |
| `auth.verify_code` | `100 total / 20 conc` | `5.0%` | `2.214 rps` | `8806.6 ms` | `9048.4 ms` | `exception_timeout=95` | Verify-code endpoint nearly saturated |
| `login.http` | `10 total / 1 conc` | `0.0%` | `0.125 rps` | `8011.3 ms` | `8026.4 ms` | `exception_timeout=10` | Failed even at single concurrency |
| `login.http` | `100 total / 20 conc` | `0.0%` | `2.491 rps` | `8019.8 ms` | `8032.2 ms` | `exception_timeout=100` | All requests timed out |
| `login.tcp` | `6 total / 1 conc` | `0.0%` | `0.125 rps` | `8011.4 ms` | `8018.0 ms` | `exception_timeout=6` | Timeout happens before TCP login succeeds |
| `login.tcp` | `12 total / 6 conc` | `0.0%` | `0.748 rps` | `8019.9 ms` | `8023.6 ms` | `exception_timeout=12` | Full timeout under moderate concurrency |
| `login.tcp` | `12 total / 12 conc` | `0.0%` | `1.496 rps` | `8011.4 ms` | `8015.8 ms` | `exception_timeout=12` | Full timeout under higher concurrency |
| `mysql_capacity` | `500 total / 20 conc` | `98.8%` | `391.940 ops/s` | `49.6 ms` | `108.4 ms` | `slow_query=3`, `exception_IntegrityError=3` | MySQL path still has usable capacity |
| `redis_capacity` | `1000 total / 50 conc` | `100.0%` | `569.312 ops/s` | `85.2 ms` | `107.7 ms` | none | Redis capacity is stronger than MySQL |

## Historical Baseline

| Scenario | Date | Config | Success Rate | Throughput | Avg Latency | P95 | Notes |
|---|---|---:|---:|---:|---:|---:|---|
| `login.http` | `2026-03-03` | `5000 total / 100 conc` | `100.0%` | `112.094 rps` | `882.4 ms` | `1107.9 ms` | Historical healthy HTTP login baseline |
| `login.tcp` | `2026-03-03` | `120 total / 1 conc` | `100.0%` | `22.107 rps` | `45.2 ms` | `50.8 ms` | Historical healthy single-connection TCP login |
| `login.tcp` | `2026-03-03` | `120 total / 6 conc` | `47.5%` | `37.747 rps` | `154.6 ms` | `240.2 ms` | Earlier concurrency instability already existed |
| `redis_capacity` | `2026-03-08` | `1000 total / 50 conc` | `100.0%` | `387.000 ops/s` | `126.1 ms` | `257.0 ms` | Historical Redis reference |

## MySQL Read/Write Analysis

### Current measured capacity

- Test shape: `read`, `write`, `mixed` round-robin, `500` total operations, `20` concurrency.
- Fresh result:
  - Overall throughput: `391.94 ops/s`
  - Overall avg latency: `49.605 ms`
  - Overall p95 latency: `108.424 ms`
  - Success rate: `98.8%`

### Per-phase latency

| Phase | Avg | P95 | Max |
|---|---:|---:|---:|
| `read` | `36.867 ms` | `95.217 ms` | `101.079 ms` |
| `write` | `52.843 ms` | `110.287 ms` | `125.186 ms` |
| `mixed` | `59.161 ms` | `111.391 ms` | `122.757 ms` |

### Interpreted bottleneck

- Read path is not the first bottleneck.
  - Private history queries use `(conv_uid_min, conv_uid_max, created_at)` indexing.
  - Group history queries use `(group_id, group_seq)` ordering.
- Write path is heavier than read path.
  - `write` and `mixed` are both slower than `read`.
  - `mysql_slow_queries = 169 / 500`, about `33.8%` of measured operations exceeded the `50 ms` slow-query threshold configured by the test.
- The main structural hotspot is group message sequence allocation.
  - Production code computes `MAX(group_seq) + 1` before insert for each group message.
  - There is a unique index on `(group_id, group_seq)`.
  - This creates a natural per-group serialization hotspot under concurrent group writes.

### Important caveat

- The `IntegrityError` seen in today’s MySQL synthetic run is mostly a test-shape artifact, not proof that MySQL itself is exhausted.
- The load script uses millisecond timestamps directly as `group_seq` in the synthetic mixed branch, which can collide under concurrency.
- Because the script opens and closes a MySQL connection per operation, the result is an end-to-end business-shaped number, not raw engine maximum throughput.

### Approximate SQL read/write rate

Given the `read/write/mixed = 1:1:1` test mix:

- Approximate read statements per second: `~261/s`
- Approximate write statements per second: `~261/s`

## Redis Analysis

- Fresh Redis result on 2026-03-11:
  - `569.312 ops/s`
  - `100%` success
  - avg `85.227 ms`
  - p95 `107.707 ms`
- Compared with current app entry failures, Redis is not the dominant bottleneck at the moment.
- Compared with current MySQL synthetic capacity, Redis still has more headroom.

## Conclusion

- Current bottleneck priority:
  1. App entry chain (`GateServer` -> backend auth/login chain), because login and verify-code now time out even at low concurrency.
  2. MySQL group message write sequencing under concurrent writes.
  3. MySQL write latency, which is higher than read latency but still far from a total outage.
  4. Redis is not the primary bottleneck in the current test results.

## Raw Reports

- `Memo_ops/runtime/loadtest/reports/http_register_20260311_155124.json`
- `Memo_ops/runtime/loadtest/reports/http_verify_code_20260311_155233.json`
- `Memo_ops/runtime/loadtest/reports/http_login_20260311_155320.json`
- `Memo_ops/runtime/loadtest/reports/http_login_20260311_155451.json`
- `Memo_ops/runtime/loadtest/reports/tcp_login_20260311_155558.json`
- `Memo_ops/runtime/loadtest/reports/tcp_login_20260311_155620.json`
- `Memo_ops/runtime/loadtest/reports/tcp_login_20260311_155636.json`
- `Memo_ops/runtime/loadtest/reports/mysql_capacity_20260311_154358.json`
- `Memo_ops/runtime/loadtest/reports/redis_capacity_20260311_154358.json`
- `local-loadtest/reports/http_login_20260303_083642.json`
- `local-loadtest/reports/tcp_login_20260303_082728.json`
- `local-loadtest/reports/tcp_login_20260303_082715.json`
