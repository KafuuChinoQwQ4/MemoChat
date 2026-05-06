# MemoChat k6 HTTP Benchmarks

Use k6 for HTTP-only baselines, then use the Python runner for full TCP login.

## Quick Start

```powershell
tools\loadtest\k6\run-http-bench.ps1 -Scenario health -Total 1000 -Concurrency 50
tools\loadtest\k6\run-http-bench.ps1 -Scenario login -Total 1000 -Concurrency 100
```

The wrapper uses a local `k6` binary when available. If not, it runs `grafana/k6`
through Docker and rewrites `127.0.0.1` targets to `host.docker.internal`.

Reports are written as JSON under `infra/Memo_ops/artifacts/loadtest/runtime/reports`
unless `config.benchmark.json` overrides `report_dir`.

## Layered Method

- `health`: `GET /healthz`, Gate HTTP baseline.
- `login`: `POST /user_login`, HTTP business login path.
- Python `tcp`: HTTP login plus ChatServer TCP ticket login.
- Python `sweep`: multiple k6 login runs across configured concurrency points.
