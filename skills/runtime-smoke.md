---
description: Run MemoChat local runtime smoke tests by deploying services, checking Docker dependencies, starting services, probing APIs, and collecting logs.
---

# MemoChat Runtime Smoke

Use when verifying that the local MemoChat stack works after code/config changes.

## Preconditions

- Build artifacts exist for the services being tested.
- Docker dependencies are running.
- No runtime service executable is locked by an old process.

## Docker Dependency Check

Run:

```powershell
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

For media or AI flows, also check MinIO, Qdrant, Neo4j, Ollama, and AI Orchestrator.

## Deploy And Start

Use:

```powershell
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
```

If deploy reports access denied, stop. Identify the locking process before retrying. Do not delete locked runtime directories repeatedly.

## Smoke Scripts

Pick relevant tests:

```powershell
tools\scripts\test_register_login.ps1
tools\scripts\test_login.ps1
tools\scripts\test_login2.ps1
tools\scripts\test_login3.ps1
tools\scripts\full_flow_test.ps1
tools\loadtest\local-loadtest-cpp\run_suite.ps1
```

For targeted API checks, use existing JSON payloads in `tools/scripts` before creating new ones.

## Logs

Collect only relevant logs:

- service stdout files under `infra/Memo_ops/runtime`
- service logs under `infra/Memo_ops/runtime/artifacts/logs`
- repository `logs/` and `artifacts/` if the running config points there
- Docker logs for dependency containers
- Loki queries through MCP when useful

## Assessment

Mark outcome:

- `PASS`: required API/runtime behavior works.
- `ENVIRONMENT_FAIL`: Docker dependency, port, credential, or file lock issue.
- `IMPLEMENTATION_FAIL`: built service starts but behavior is wrong.
- `INCONCLUSIVE`: logs or tests are insufficient.

## Cleanup

Use:

```powershell
tools\scripts\status\stop-all-services.bat
```

Leave Docker dependencies running unless the user asks to stop them.
