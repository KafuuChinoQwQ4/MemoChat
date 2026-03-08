# MemoChat ELK (Compatibility Mode)

This stack remains available during migration. The default local observability stack is now under `observability/otel`.

## Start

```powershell
cd observability/elk
docker compose up -d
```

- Elasticsearch: http://localhost:9200
- Kibana: http://localhost:5601
- Logstash API: http://localhost:9600

## Collected log paths

- `build/run/*/logs/*.json`
- `build/bin/Release/logs/*.json`
- `build/bin/Debug/logs/*.json`
- `local-loadtest/logs/*.json`
- `server/VarifyServer/logs/*.json`

## Index

- `memochat-logs-YYYY.MM.dd`

## Stop

```powershell
cd observability/elk
docker compose down
```
