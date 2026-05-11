---
description: Run MemoChat local runtime smoke tests by deploying services, checking Docker dependencies, starting services, probing APIs, and collecting logs.
---

# MemoChat Runtime Smoke

Use when verifying that the local MemoChat stack works after code/config changes.

## Preconditions

- Arch/WSL Linux server artifacts exist in `build-linux-server-gcc16/bin` for the services being tested.
- For any fresh Linux server code change, first run `cmake --preset linux-server-gcc16` and `cmake --build --preset linux-server-gcc16 --parallel 12`; `deploy_services.sh` copies from that build output by default.
- Docker dependencies are running under Arch native Docker. Source `/root/.memochat-linux-env` before compose commands so `DOCKER_HOST` is unset.
- No old Linux runtime service process is still bound to the target ports.

## Docker Dependency Check

Run:

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

For media or AI flows, also check MinIO, Qdrant, Neo4j, Ollama, and AI Orchestrator.

## Deploy And Start

Use:

```bash
source /root/.memochat-linux-env
cmake --preset linux-server-gcc16
cmake --build --preset linux-server-gcc16 --parallel 12
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

If start reports ports already listening, run `tools/scripts/status/stop-all-services.sh` or identify the owning process before retrying. Do not stop Docker dependencies unless the user asks.

## Smoke Scripts

Pick relevant tests:

```powershell
tools/scripts/test_register_login.ps1
tools/scripts/test_login.ps1
tools/scripts/test_login2.ps1
tools/scripts/test_login3.ps1
tools/scripts/full_flow_test.ps1
python tools/loadtest/python-loadtest/py_loadtest.py --config tools/loadtest/python-loadtest/config.json --scenario all --total 20 --concurrency 5
```

These probes are still PowerShell-first; run them from Windows when needed. For targeted API checks, use existing JSON payloads in `tools/scripts` before creating new ones.

## Logs

Collect only relevant logs:

- service stdout/stderr files under `infra/Memo_ops/artifacts/logs/services`
- pid files under `infra/Memo_ops/runtime/pids`
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

```bash
tools/scripts/status/stop-all-services.sh
```

Leave Docker dependencies running unless the user asks to stop them.
