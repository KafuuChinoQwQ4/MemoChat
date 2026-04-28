# Verification Result

Commands:
- `tools\scripts\status\deploy_services.bat`
- `tools\scripts\status\start-all-services.bat`
- PowerShell parser checks for `deploy_services.ps1`, `run-service-console.ps1`, and `patch_chat_configs.ps1`
- `docker compose -f infra\deploy\local\compose\datastores.yml -f infra\deploy\local\compose\nginx-lb.yml config --quiet`
- `git diff --check` on touched files
- `rg --pcre2` for stale four-node cluster strings and conflicting `50060` / `Http2Port=8086`
- `Get-NetTCPConnection` for runtime ports

Outcome:
- Static checks: PASS
- Docker compose combined config: PASS
- Runtime deployment from existing build artifacts: PASS
- Runtime startup: PASS

Observed listening ports:
- GateServer1: `8080`, `8082`
- GateServer2: `8084`, `8085`
- VarifyServer health: `8083`
- StatusServer: `50052`
- ChatServer TCP: `8090`, `8091`, `8092`, `8093`, `8094`, `8097`
- ChatServer RPC: `50055`, `50056`, `50057`, `50058`, `50059`, `50381`

Notes:
- `ChatServer6` initially failed on RPC `50060` because Windows had reserved `50060-50159`; it was moved to `50381`.
- `GateServer2` HTTP/2 was moved from `8086` to `8085` to avoid the local InfluxDB port.

