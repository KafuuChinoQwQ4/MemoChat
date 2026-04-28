# Context

Task: debug and fix the reported crash where `start-all-services.bat` eventually causes `ChatServer` and `GateServer` to crash.

Relevant files:
- `apps/server/core/GateServer/GateServer.cpp`
- `apps/server/core/GateServer/NgHttp2Server.cpp`
- `apps/server/core/ChatServer/ChatIngressCoordinator.cpp`
- `apps/server/core/ChatServer/CServer.cpp`
- `apps/server/core/ChatServer/CServer.h`
- `apps/server/core/ChatServer/CSession.cpp`
- `apps/server/core/ChatServer/CSession.h`

Dependencies / runtime:
- Docker containers were already running and healthy for Redis, Postgres, RabbitMQ, Redpanda, MinIO, Neo4j, Qdrant, Ollama, Prometheus, Loki, Tempo, Grafana, InfluxDB, and cAdvisor.
- Local services are launched through `tools\scripts\status\start-all-services.bat` and stopped through `tools\scripts\status\stop-all-services.bat`.

Checks performed:
- `docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"`
- `docker exec memochat-redis redis-cli -a 123456 ping`
- `docker exec memochat-postgres pg_isready -U memochat -d memo_pg`
- `docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping`
- `docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092`
- Windows Event Log `Application` for `APPCRASH` entries
- Runtime logs under `infra/Memo_ops/runtime/artifacts/logs/services`

Root cause found:
- `GateServer` HTTP/2 response headers used `std::to_string(...).c_str()` temporary pointers in nghttp2 header submission, which is a use-after-free hazard and matches the observed `c0000005` crashes.
- `ChatServer` TCP accept/timer/session shutdown path used raw `this` callbacks without explicit stop gating. I added stop state and null-server guarding so shutdown does not leave callbacks pointing at a torn-down server.

Verification commands:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- `tools\scripts\status\start-all-services.bat`
- `tools\scripts\test_register_login.ps1`

Open risks:
- QUIC/HTTP3 certificate load still logs `ConfigurationLoadCredential (FILE_PROTECTED)` failures, but the services continue running through HTTP/1.1 and HTTP/2 fallback.
- The smoke script still reports a client-side connection error on the verify-code call; the servers stay alive and do not crash during the post-fix observation window.
