---
description: 通过 Prometheus、Loki、Tempo、Grafana、InfluxDB、cAdvisor 和 Docker 调查 MemoChat 指标、日志、链路、仪表盘和容器资源使用情况。
---

# MemoChat 可观测性

用于诊断延迟、错误、缺失遥测、服务健康、仪表盘、日志、链路或资源使用问题。

## 数据源

可用时优先使用 MCP 工具：

- Prometheus：`prometheus_query`、`prometheus_targets`
- Loki：`loki_query`
- Tempo：`tempo_services`、`tempo_search_traces`
- Grafana：`grafana_datasources`、`grafana_search_dashboards`
- InfluxDB：`influx_query`
- cAdvisor：`cadvisor_metrics`

备用 HTTP 端点：

```bash
curl -fsS http://127.0.0.1:9090/-/ready
curl -fsS http://127.0.0.1:9093/-/ready
curl -fsS http://127.0.0.1:3100/ready
curl -fsS http://127.0.0.1:3200/ready
curl -fsS http://127.0.0.1:3000/api/health
curl -fsS http://127.0.0.1:8088/metrics
```

当前本地观测基础设施运行在 `archlinux` WSL 的 Arch 原生 Docker 中，固定端口包括 Prometheus `9090`、Alertmanager `9093`、Grafana `3000`、Loki `3100`、Tempo `3200`、OTel Collector `4317/4318/9411/9464`、InfluxDB `8086`、cAdvisor `8088`。

## 调查流程

1. 确定服务和时间窗口。
2. 检查 Docker 状态和重启历史。
3. 检查 Prometheus targets 是否有 scrape 失败。
4. 查询 Loki 中的服务错误。
5. 按服务或端点在 Tempo 中搜索 trace。
6. 延迟或崩溃相关时，检查 cAdvisor/container CPU/内存。
7. 只有仪表盘错误时，才检查 Grafana datasource/dashboard provisioning。

## 常用查询模式

Prometheus：

```text
up
rate(http_server_duration_count[5m])
process_resident_memory_bytes
container_cpu_usage_seconds_total
```

Loki：

```text
{service="GateServer"} |= "error"
{service="ChatServer"} |= "warn"
{service=~"GateServer|ChatServer|StatusServer|VarifyServer"}
```

Tempo：

- 先按服务名搜索
- 再按 route、status 或日志中的 trace id 收窄

## 代码/配置区域

检查：

- 服务 `config.ini` 的 telemetry sections
- `apps/server/core/common/logging`
- 若存在，检查 `apps/server/core/common/observability`
- `infra/deploy/local/observability`
- `infra/deploy/local/docker-compose.yml`
- `infra/Memo_ops/runtime/services/*/config.ini`

## 报告

包含：

- 准确时间窗口
- 查询过的数据源
- 查询字符串
- 关键日志/指标/trace 发现
- 可能根因
- 推荐的代码/配置/运行时修复
