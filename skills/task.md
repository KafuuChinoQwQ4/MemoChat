---
description: Implement a MemoChat feature or fix through context, plan, implementation, Docker-backed verification, and review.
---

# MemoChat Task

Use for normal implementation work in `/root/code/MemoChat-Qml-Drogon-linux`. Treat `D:\MemoChat-Qml-Drogon` as the legacy Windows checkout unless the user explicitly asks for Windows work.

Default to the Controller-led parallel workflow from `parallel-agents.md` for implementation tasks. The Controller owns architecture, plan, contracts, worker dispatch, integration, and final acceptance. After context and first contracts are clear, dispatch safe worker lanes by default. Local-only execution is an exception, allowed only when the active tool/policy environment forbids workers, the user explicitly asks for single-agent work, the task is genuinely tiny and has no useful test/review lane, the task is strictly sequential, or no safe split exists; record the exact reason in `plan.md` before implementation.

## Invocation

Treat `$ARGUMENTS` as the task. If it starts with an existing `.ai/<name>/about.md`, treat the first token as a follow-up project name and use the remaining text as the new task.

## Required Workflow

1. Create `.ai/<project>/<letter>/`.
2. Gather context into `context.md`.
3. Write and assess `plan.md`.
4. Open concurrency by default:
   - spawn workers for disjoint useful lanes when permitted
   - keep Controller in charge of contracts and final acceptance
   - record a local-only reason before implementation when worker dispatch is blocked, unsafe, or not useful
5. Implement one plan phase at a time.
6. Verify with the narrowest relevant build/test/runtime command.
7. Review the diff and fix important issues.
8. Finish with a concise status summary.

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

```bash
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

Record any query that affected your reasoning in `context.md` or verification logs.

## Build Selection

Use the Linux GCC16 presets for code changes that will be deployed or runtime-tested in Arch Linux WSL. `deploy_services.sh` copies server artifacts from `build-linux-server-gcc16/bin` by default.

```bash
source /root/.memochat-linux-env
cmake --preset linux-server-gcc16
cmake --build --preset linux-server-gcc16 --parallel 12
```

For test-only checks, run tests from the full build tree:

```bash
ctest --preset linux-server-gcc16 --output-on-failure
```

For Linux client checks use `linux-client-gcc16`; for cross-stack Linux checks use `linux-full-gcc16`.

## Runtime Scripts

Use existing scripts instead of inventing new orchestration:

```bash
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
tools/scripts/status/stop-all-services.sh
```

Use the `.bat`/`.ps1` scripts only for legacy Windows runtime/client checks. Existing PowerShell smoke probes may still be useful from Windows after Linux services are running.

## Implementation Rules

- Prefer existing helpers and module boundaries.
- Keep server/client protocol and config changes synchronized.
- Add migrations or init changes when persistent schema changes are required.
- Keep Linux generated/downloaded heavy files under `/data`; keep Arch Docker bind data under `/data/docker-data/memochat`; use `D:` only when operating on legacy Windows/Docker Desktop data.
- Do not revert user work.
- Avoid broad formatting churn.

## Completion

Report:

- files changed
- concurrency lanes used by default, or the exact local-only/blocker reason
- verification commands and outcomes
- Docker/MCP checks performed
- known blockers or residual risk
- `.ai` project name for follow-up
