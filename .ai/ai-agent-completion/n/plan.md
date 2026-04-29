# Plan

Task summary: wire AI Agent alerts to Alertmanager and add log/trace drill-down affordances to Grafana.

Files to modify/create:

- `infra/deploy/local/docker-compose.yml`
- `infra/deploy/local/observability/alertmanager/alertmanager.yml`
- `infra/deploy/local/observability/prometheus/prometheus.yml`
- `infra/deploy/local/observability/loki/config.yaml`
- `infra/deploy/local/observability/grafana/dashboards/ai-agent-overview.json`
- `infra/Memo_ops/server/ops_server/routes/metrics.py`
- `.ai/ai-agent-completion/n/*`

Implementation phases:

- [x] Gather context and inspect existing alert/log/trace wiring.
- [x] Add Alertmanager service and config.
- [x] Wire Prometheus/Loki to Alertmanager.
- [x] Add MemoOps Alertmanager webhook receiver and alert merge.
- [x] Add Grafana log/trace drill-down links/panels.
- [x] Verify runtime config and record review.

Assessed: yes
