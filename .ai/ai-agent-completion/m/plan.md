# Plan

Task summary: add production-oriented AI Agent observability and default CI hardening without making CI depend on local Docker/Ollama runtime.

Approach:

- Use Prometheus alerting rules over the metrics already exposed by AIOrchestrator.
- Add a focused Grafana dashboard JSON under the existing provisioning folder.
- Add a small AI Agent CI job to `.github/workflows/ci.yml`.
- Add a minimal Python CI requirements file for offline AIOrchestrator unit tests.
- Record verification and review in this phase folder.

Files to modify/create:

- `.github/workflows/ci.yml`
- `apps/server/core/AIOrchestrator/requirements-ci.txt`
- `infra/deploy/local/docker-compose.yml`
- `infra/deploy/local/observability/prometheus/prometheus.yml`
- `infra/deploy/local/observability/prometheus/rules/ai_orchestrator.yml`
- `infra/deploy/local/observability/grafana/dashboards/ai-agent-overview.json`
- `.ai/ai-agent-completion/m/*`

Implementation phases:

- [x] Gather CI and observability context.
- [x] Add Prometheus AI alert rules and mount them into local Prometheus.
- [x] Add Grafana AI Agent overview dashboard.
- [x] Add offline AI Agent CI job and minimal Python requirements.
- [x] Verify config syntax, Python tests, Prometheus rule loading, and Grafana provisioning.
- [x] Review diff and record completion.

Assessed: yes
