# MemoChat OTel Stack

## Start

```powershell
cd observability/otel
docker compose up -d
```

- Grafana: http://localhost:3000
- Tempo API: http://localhost:3200
- Loki API: http://localhost:3100
- OTel Collector OTLP HTTP: http://localhost:4318
- OTel Collector Zipkin: http://localhost:9411/api/v2/spans

## What It Collects

- Service and client JSON log files from the repository workspace via the collector `filelog` receiver
- Zipkin-compatible spans exported by GateServer, StatusServer, ChatServer, ChatServer2, VarifyServer, QML client, and local load tests
- Future OTLP telemetry on `4317` and `4318`

## Notes

- This stack is now the default local observability path.
- The ELK stack under `observability/elk` remains for compatibility during migration.
