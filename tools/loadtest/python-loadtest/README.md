# MemoChat Layered Load Test

MemoChat uses a layered load-test workflow:

- `k6` for HTTP-only baselines and login throughput.
- Python for full TCP login because it speaks the MemoChat frame protocol.
- `sweep` for comparing concurrency points instead of trusting one headline QPS.

## Quick Start

```powershell
tools\loadtest\run-layered-suite.ps1 -Total 200 -Concurrency 20
tools\loadtest\run-layered-suite.ps1 -Cases http-health,http-login,tcp-login -Total 1000 -Concurrency 100
tools\loadtest\run-layered-suite.ps1 -Cases redis,postgres,mongo,qdrant -Total 1000 -Concurrency 100
tools\loadtest\k6\run-http-bench.ps1 -Scenario health -ConfigPath tools\loadtest\python-loadtest\config.benchmark.json -Total 1000 -Concurrency 50
tools\loadtest\k6\run-http-bench.ps1 -Scenario login -ConfigPath tools\loadtest\python-loadtest\config.benchmark.json -Total 1000 -Concurrency 100
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.benchmark.json --scenario tcp --total 500 --concurrency 50
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.benchmark.json --scenario sweep --total 200
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario ai-health --total 50 --concurrency 10
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario rag --total 10 --concurrency 2
```

## Scenarios

| Scenario | What it measures |
|---|---|
| `run-layered-suite.ps1` | Orchestrates split app/dependency cases and writes one suite JSON plus one Markdown table |
| `health` via k6 | GateServer `GET /healthz` HTTP baseline |
| `http` | GateServer `/user_login` through k6 by default; set `use_k6_http=false` to use the Python fallback |
| `tcp` | Gate login plus ChatServer TCP ticket login |
| `sweep` | Multiple k6 login runs across `sweep_concurrency` from config |
| `ai-health` | AIOrchestrator `/health` latency and availability |
| `agent` | AI Agent `/agent/run` latency and success rate |
| `rag` | Knowledge-base upload/search latency, recall@1/@3/@5/@10, top-k misses, and simple hallucination proxy |
| `all` | Runs every scenario above |

Reports are JSON files written to `report_dir` from `config.json` unless
`--report-dir` is supplied.

The RAG scenario is intentionally harsher than a smoke test. By default it
uploads 50 documents: three topic-dense target documents plus 47 synthetic
near-miss distractor documents. It runs paraphrased queries, requests top5
results, evaluates recall at several cutoffs, uses an isolated synthetic uid to
avoid old Qdrant collections, and deletes the uploaded knowledge bases after the
run. Treat a 100% recall result as something to inspect in `details.rag_misses`
and the per-cutoff quality fields, not as proof from one tiny document.

## Layered Suite Cases

`tools\loadtest\run-layered-suite.ps1` is the main benchmark entrypoint for
splitting the system into measurable layers:

| Case | Layer | Tool | Output |
|---|---|---|---|
| `http-health` | app baseline | k6 | QPS, failure rate, p50/p90/p95/p99 for `GET /healthz` |
| `http-login` | app business | k6 | QPS, failure rate, p50/p90/p95/p99 for `/user_login` |
| `tcp-login` | app business | Python protocol client | QPS, p50/p75/p90/p95/p99 for HTTP login plus ChatServer TCP login |
| `redis` | dependency | Python client | separate SET and GET QPS/p99 |
| `postgres` | dependency | Python client | separate UPSERT and primary-key read QPS/p99 |
| `mongo` | dependency | Python client | separate insert and findOne QPS/p99 |
| `qdrant` | dependency | HTTP client | separate vector upsert and top-k search QPS/p99 |
| `ai-health` | app dependency service | Python HTTP client | AIOrchestrator health QPS/p99 |
| `agent`, `rag` | deep AI path | Python HTTP client | optional slow-path AI and RAG smoke benchmarks |

Examples:

```powershell
# Conservative smoke, good before every real run.
tools\loadtest\run-layered-suite.ps1 -Total 200 -Concurrency 20

# App login paths only.
tools\loadtest\run-layered-suite.ps1 -Cases http-health,http-login,tcp-login -Total 2000 -Concurrency 100

# Dependency read/write baselines only.
tools\loadtest\run-layered-suite.ps1 -Cases redis,postgres,mongo,qdrant -Total 2000 -Concurrency 100

# Larger default run.
tools\loadtest\run-layered-suite.ps1 -Heavy
```

The suite continues after individual case failures. This is intentional: if
GateServer is down, dependency read/write benchmarks still run and the final
Markdown summary clearly separates environment failures from dependency
capacity.

## How To Read Results

Compare layers in this order:

1. `health`: framework and Windows networking baseline.
2. `http`: login JSON, Redis token, login cache/Postgres, and telemetry overhead.
3. `tcp`: HTTP login plus ChatServer TCP auth and downstream Status/Redis work.
4. `sweep`: where latency starts bending sharply as concurrency grows.

Do not compare MemoChat `tcp` directly with framework hello-world benchmarks.
They measure different work.
