# Verification Result

Date: 2026-04-27T09:06:52-07:00

Static checks:
- PASS: `tools\scripts\status\deploy_services.ps1` PowerShell parse.
- PASS: `tools\scripts\status\run-service-console.ps1` PowerShell parse.
- PASS: `tools\scripts\patch_chat_configs.ps1` PowerShell parse.
- PASS: `git diff --check` on touched files.
- PASS: `docker compose -f infra\deploy\local\compose\datastores.yml -f infra\deploy\local\compose\nginx-lb.yml config --quiet`.
- PASS: `start-all-services.bat` and `stop-all-services.bat` normalized to CRLF with `bareLF=0`.

Docker dependency checks:
- PASS: Redis `PONG`.
- PASS: Postgres `select 1`.
- PASS: RabbitMQ ping.
- PASS: Redpanda cluster info.

Runtime deployment:
- PASS: `tools\scripts\status\stop-all-services.bat`.
- PASS: `tools\scripts\status\deploy_services.bat`.
- PASS: `tools\scripts\status\start-all-services.bat`.

Runtime listeners:
- GateServer1: HTTP `8080`, HTTP/2 `8082`.
- GateServer2: HTTP `8084`, HTTP/2 `8085`.
- VarifyServer1: gRPC `50051`, health `8083`.
- VarifyServer2: gRPC `50383`, health `8087`.
- StatusServer1: gRPC `50052`.
- StatusServer2: gRPC `50382`.
- ChatServer TCP: `8090`, `8091`, `8092`, `8093`, `8094`, `8097`.
- ChatServer RPC: `50055`, `50056`, `50057`, `50058`, `50059`, `50381`.

HTTP smoke:
- PASS: `http://127.0.0.1:8083/healthz`.
- PASS: `http://127.0.0.1:8087/healthz`.
- PASS: `http://127.0.0.1:8080/healthz`.
- PASS: `http://127.0.0.1:8084/healthz`.

Notes:
- No full build was run, per user preference. Existing `build\bin\Release` artifacts were used.
