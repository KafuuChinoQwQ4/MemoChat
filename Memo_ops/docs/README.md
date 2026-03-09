# Memo_ops

`Memo_ops` is an isolated observability and load-test workspace for MemoChat.

## Components

- `server/ops_server`: FastAPI query API for overview, load tests, logs, traces, and metrics
- `server/ops_collector`: background importer and service-level metrics collector
- `client/MemoOps-qml`: standalone QML desktop dashboard
- `config/`: isolated YAML and INI configs

## Data Isolation

- MySQL schema: `memo_ops`
- Redis prefix: `ops:*`
- Raw business data remains in `memo`
- Raw log bodies and traces stay in Loki and Tempo

## Quick Start

```powershell
python Memo_ops/scripts/init_memo_ops_schema.py
powershell -ExecutionPolicy Bypass -File Memo_ops/scripts/start_ops_platform.ps1
```

To stop:

```powershell
powershell -ExecutionPolicy Bypass -File Memo_ops/scripts/stop_ops_platform.ps1
```

## Ports

- OpsServer: `18080`
- OpsCollector Prometheus endpoint: `19100`
- Grafana: `3000`
- Loki: `3100`
- Tempo: `3200`

## Notes

- `Memo_ops` can start against the same MySQL instance as `memo`, as long as it uses a separate schema.
- To move `Memo_ops` to another MySQL server later, change only `Memo_ops/config/*.yaml`.
- `Memo_ops` build support is fixed to `MSVC2022 + Ninja Multi-Config + Qt 6.8.3 msvc2022_64`.
- Verified toolchain paths:
  - `D:/VS2026stable/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe`
  - `D:/qt/6.8.3/msvc2022_64`
  - `D:/ninja-win/ninja.exe`

## Build Validation

```powershell
Set-Location Memo_ops
cmake --preset msvc2022-memo-ops
cmake --build --preset msvc2022-memo-ops-release --target MemoOpsQml
Set-Location ..
python -m Memo_ops.server.ops_collector.main --config Memo_ops/config/opscollector.yaml --once
```
