# Context

Task: continue AI Agent productionization by wiring Alertmanager and adding Loki/Tempo drill-down surfaces after phase `m`.

Existing state:

- Prometheus has AIOrchestrator alert rules in `infra/deploy/local/observability/prometheus/rules/ai_orchestrator.yml`.
- Grafana has `AI Agent Overview` dashboard provisioned.
- Local compose has Prometheus, Grafana, Loki, Tempo, OTel collector, but no Alertmanager.
- Loki config currently points ruler alerts to `http://localhost:9093`, which is not valid from inside the Loki container.
- MemoOps exposes `GET /api/alerts` but no Alertmanager webhook receiver.

Implementation target:

- Add `memochat-alertmanager` on stable port `9093`.
- Add local Alertmanager config with a webhook receiver to MemoOps.
- Point Prometheus alerting and Loki ruler to `memochat-alertmanager:9093`.
- Add MemoOps `POST /api/alerts/alertmanager` to normalize Alertmanager webhook payloads into Redis.
- Merge Alertmanager alerts into `GET /api/alerts`.
- Add Grafana dashboard links/panels for Loki logs and Tempo traces.

Verification:

- Parse YAML/JSON/Python.
- Start/recreate Prometheus, Loki, Alertmanager, Grafana.
- Run `promtool check config`.
- Check Alertmanager `/-/ready`.
- Check Prometheus alertmanager discovery.
- Check Grafana dashboard provisioning.
