# MemoChat Load Test Toolkit — Legacy Entry Point

> **This directory is kept for backwards compatibility only.**
> All tools, documentation, and scripts have moved to `local-loadtest-cpp/`.

## What changed

`local-loadtest/` and `local-loadtest-cpp/` have been merged into `local-loadtest-cpp/`.

| What | Where |
|---|---|
| C++ binary (`memochat_loadtest.exe`) | `local-loadtest-cpp/build/Release/memochat_loadtest.exe` |
| Unified config | `local-loadtest-cpp/config.json` |
| PowerShell suite runner | `local-loadtest-cpp/run_suite.ps1` |
| Protocol comparison tools | `local-loadtest-cpp/compare_protocols.py`, `run_protocol_comparison.py` |
| C++ binary wrapper | `local-loadtest-cpp/cpp_run.py` |
| Python business scripts | Still in `local-loadtest/*.py` |

## Quick migration

**Old:**
```powershell
cd local-loadtest
.\run_suite.ps1 -Scenario all
```

**New:**
```powershell
cd local-loadtest-cpp
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario all
```

## Running the C++ binary directly

```powershell
cd local-loadtest-cpp
.\build\Release\memochat_loadtest.exe --scenario tcp --config config.json
.\build\Release\memochat_loadtest.exe --scenario quic --config config.json
```

## One-shot protocol comparison

```powershell
cd local-loadtest-cpp
python run_protocol_comparison.py --total 500 --concurrency 50
```

## Full documentation

See `local-loadtest-cpp/README.md` for the complete documentation.
