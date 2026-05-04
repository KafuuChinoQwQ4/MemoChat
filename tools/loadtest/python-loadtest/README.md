# MemoChat Python Load Test

Python-only load testing entrypoint for MemoChat. It replaces the old native C++
load-test binary and does not require CMake, MSVC, WinHTTP, or msquic builds.

## Quick Start

```powershell
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario http --total 100 --concurrency 20
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario tcp --total 100 --concurrency 20
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario ai-health --total 50 --concurrency 10
python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario rag --total 10 --concurrency 2
```

## Scenarios

| Scenario | What it measures |
|---|---|
| `http` | GateServer `/user_login` latency, success rate, and wall-clock throughput |
| `tcp` | Gate login plus ChatServer TCP ticket login |
| `ai-health` | AIOrchestrator `/health` latency and availability |
| `agent` | AI Agent `/agent/run` latency and success rate |
| `rag` | Knowledge-base upload/search latency, recall@k, and simple hallucination proxy |
| `all` | Runs every scenario above |

Reports are JSON files written to `report_dir` from `config.json` unless
`--report-dir` is supplied.
