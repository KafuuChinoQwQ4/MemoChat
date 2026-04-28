# Context

Task:
- Change local MemoChat backend topology from one GateServer and four ChatServer instances to two GateServer instances and six ChatServer instances.

Relevant files:
- `tools/scripts/status/start-all-services.bat`
- `tools/scripts/status/stop-all-services.bat`
- `tools/scripts/status/deploy_services.bat`
- `apps/server/core/GateServer/config.ini`
- `apps/server/core/GateServer/gate2.ini`
- `apps/server/core/StatusServer/config.ini`
- `apps/server/core/ChatServer/config.ini`
- `apps/server/core/ChatServer/chatserver*.ini`
- `infra/Memo_ops/runtime/services/*/config.ini`
- `infra/deploy/local/compose/nginx-lb.yml`
- `infra/deploy/local/compose/nginx.conf`
- `infra/deploy/local/README.md`

Port plan:
- GateServer1: HTTP `8080`, HTTP/2 `8082`
- GateServer2: HTTP `8084`, HTTP/2 `8085`
- ChatServer1-4: existing TCP `8090-8093`, QUIC `8190-8193`, RPC `50055-50058`
- ChatServer5: TCP `8094`, QUIC `8194`, RPC `50059`
- ChatServer6: TCP `8097`, QUIC `8195`, RPC `50381`

Notes:
- `8095` and `8096` are already used by `AIServer` and `AIOrchestrator`, so ChatServer6 intentionally uses `8097`.
- `8086` is already published by InfluxDB, so GateServer2 HTTP/2 uses `8085`.
- Windows reserves `50060-50159` on this machine, so ChatServer6 RPC uses `50381`.
- Docker dependency ports are unchanged.
- Current services were running while context was gathered, so runtime edits must avoid assuming deployed executables can be overwritten in place.
