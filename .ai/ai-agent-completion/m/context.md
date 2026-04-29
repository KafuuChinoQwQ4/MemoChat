# Context

Task: productionize AI Agent observability and CI hardening after the backend/runtime AI Agent chain passed local smoke.

Relevant existing state:

- Existing project artifacts are under `.ai/ai-agent-completion/`.
- Phase `l` already added:
  - `tools/scripts/ai_agent_smoke.ps1`
  - AIOrchestrator `/metrics`
  - Prometheus scrape job `ai_orchestrator`
  - Ollama storage on `D:/docker-data/memochat/ollama`
  - Gate/AIServer metadata passthrough for bounded smoke generation
- Local observability stack is Docker-based under `infra/deploy/local/docker-compose.yml`.
- Grafana dashboards are provisioned from `infra/deploy/local/observability/grafana/dashboards`.
- Prometheus config is `infra/deploy/local/observability/prometheus/prometheus.yml`.
- CI workflow is `.github/workflows/ci.yml`.

Constraints:

- Do not put real Ollama/Gate smoke into default CI; it depends on running Docker services and local model data.
- Keep ports unchanged.
- Keep large caches/models on `D:`.
- Use existing Prometheus/Grafana provisioning layout.

Implementation target:

- Add Prometheus rule file for AIOrchestrator availability, error-rate, and retry signals.
- Mount/rule-load the Prometheus rule file in local compose.
- Add Grafana dashboard for AI Agent route health and Ollama recovery signals.
- Add CI job for offline AI Agent regressions:
  - AIOrchestrator Python unit tests
  - `ai_agent_smoke.ps1` syntax parse
  - no live Docker/Ollama dependency

Verification:

- YAML/JSON parse checks.
- Run AIOrchestrator Python unit tests locally.
- Reload Prometheus and query rule health if possible.
- Check Grafana dashboard provisioning through Grafana search if possible.
