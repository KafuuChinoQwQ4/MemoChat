---
description: Implement a MemoChat feature or fix through context, plan, implementation, Docker-backed verification, and review.
---

# MemoChat Task

Use for normal implementation work in `D:\MemoChat-Qml-Drogon`.

## Invocation

Treat `$ARGUMENTS` as the task. If it starts with an existing `.ai/<name>/about.md`, treat the first token as a follow-up project name and use the remaining text as the new task.

## Required Workflow

1. Create `.ai/<project>/<letter>/`.
2. Gather context into `context.md`.
3. Write and assess `plan.md`.
4. Implement one plan phase at a time.
5. Verify with the narrowest relevant build/test/runtime command.
6. Review the diff and fix important issues.
7. Finish with a concise status summary.

## MemoChat Context Checklist

Always account for the relevant layers:

- C++ server: `apps/server/core`.
- QML clients: `apps/client/desktop` and `infra/Memo_ops/client`.
- Runtime config: `apps/server/config`, `infra/Memo_ops/config`, deployed configs under runtime service folders.
- Database migrations: `apps/server/migrations/postgresql`.
- Local Docker: `infra/deploy/local/docker-compose.yml` and compose fragments under `infra/deploy/local/compose`.
- Scripts: `tools/scripts` and `tools/scripts/status`.
- Tests/load tools: `tests`, `apps/server/core/common/*/tests`, `tools/loadtest/python-loadtest`.

## Docker And MCP Rules

- Containers are the source of truth for infrastructure.
- Do not install or start local Redis/Postgres/Mongo/RabbitMQ/Redpanda/etc. outside Docker.
- Do not change stable port mappings unless explicitly requested.
- Use `docker ps` and MCP tools to inspect state.
- Prefer Docker commands for direct checks:

```powershell
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

Record any query that affected your reasoning in `context.md` or verification logs.

## Build Selection

Use the full local build for code changes that will be deployed or runtime-tested. `deploy_services.bat` copies only from `build\bin\Release`, which is produced by `msvc2022-full`; do not rely on `build-verify-server` or `build-verify-client` for runtime validation.

```powershell
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
```

For test-only checks, run tests from the full build tree:

```powershell
ctest --preset msvc2022-full
```

If a task needs runtime smoke, run the full build first, then deploy from `build\bin\Release`.

## Runtime Scripts

Use existing scripts instead of inventing new orchestration:

```powershell
tools\scripts\preflight.ps1
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
tools\scripts\status\stop-all-services.bat
tools\scripts\test_register_login.ps1
tools\scripts\test_login.ps1
tools\scripts\full_flow_test.ps1
```

If deploy/start scripts hit access denied, identify the running service process and ask the user to close or stop it before retrying.

## Implementation Rules

- Prefer existing helpers and module boundaries.
- Keep server/client protocol and config changes synchronized.
- Add migrations or init changes when persistent schema changes are required.
- Keep generated/downloaded heavy files on `D:`.
- Do not revert user work.
- Avoid broad formatting churn.

## Completion

Report:

- files changed
- verification commands and outcomes
- Docker/MCP checks performed
- known blockers or residual risk
- `.ai` project name for follow-up
