---
name: memochat-task-think
description: Orchestrate a multi-phase implementation workflow for MemoChat-Qml-Drogon. Use when a task needs structured context gathering, planning, implementation, Docker-backed data checks, CMake build verification, review, and persistent artifacts under the local .ai project folders.
---

# MemoChat Task Pipeline

Use this skill to drive non-trivial changes in `D:\MemoChat-Qml-Drogon` while keeping the main thread thin and preserving useful artifacts.

## Project Rules

- Treat Windows PowerShell as the default shell.
- Keep all infrastructure dependencies in Docker. Check containers with `docker ps`; inspect databases and queues through Docker or the configured MCP tools, not local host installs.
- Keep container ports stable. Do not change compose port mappings unless the task explicitly asks for it.
- Prefer downloading caches, models, packages, and generated large artifacts to `D:` paths.
- Use MCP tools when available:
  - Docker services: Prometheus, Loki, Tempo, Grafana, MinIO, RabbitMQ, InfluxDB, cAdvisor.
  - Datastores: MongoDB, Neo4j, Qdrant, Redpanda, Postgres, Redis as configured.
- Never reset or revert user changes. Work with the dirty tree.

## Artifact Layout

Create or reuse:

```text
.ai/<project-name>/
  about.md
  a/
    context.md
    plan.md
    review1.md
    logs/
      phase-<name>.prompt.md
      phase-<name>.progress.md
      phase-<name>.result.md
  b/
    ...
```

- `about.md` is the project-level blueprint written as if the feature exists and works.
- `<letter>/context.md` is the self-contained task context for downstream agents.
- `<letter>/plan.md` is the executable plan and status checklist.
- `logs/` records prompts, progress, verification output, and blockers.

## Phase 0: Setup

1. Record start time with `Get-Date`.
2. Detect follow-up work:
   - If the first token of the request matches `.ai/<token>/about.md`, use that project.
   - Otherwise create a short kebab-case project name.
3. Pick the next task letter (`a`, `b`, ...), create directories, and write the task request to `logs/phase-setup.result.md`.
4. Capture any screenshot or attachment summary into `context.md` or a log file before delegation.

## Phase 1: Context

Gather context before planning.

Read only what is relevant, but include enough for a fresh agent to work without the original chat:

- `CMakeLists.txt`, `CMakePresets.json`, and nearest module `CMakeLists.txt`.
- `apps/server/core/*` for C++ services:
  `GateServer`, `GateServerCore`, `ChatServer`, `StatusServer`, `VarifyServer`, `AIServer`, `AIOrchestrator`, `common`.
- `apps/client/desktop/*` and `infra/Memo_ops/client/*` for QML/client work.
- `apps/server/config`, `apps/server/migrations/postgresql`, `infra/deploy/local`, and `tools/scripts` for runtime/database work.
- Existing similar code paths, tests, scripts, and configs.

Write:

- `.ai/<project>/about.md`
- `.ai/<project>/<letter>/context.md`

`context.md` must include task description, relevant files, data/service dependencies, Docker containers/ports touched, MCP/database queries used, build/test commands, and open risks.

## Phase 2: Plan

Write `.ai/<project>/<letter>/plan.md` with:

- task summary
- approach
- files to modify/create
- implementation phases with concrete file/function steps
- Docker/MCP checks required
- build/test commands
- status checklist

Keep phases small enough that one agent or one focused pass can complete each phase.

## Phase 3: Plan Assessment

Review the plan against the actual code before editing:

- file paths and symbols exist
- database/container assumptions match Docker state
- schema/migration changes include init and runtime impact
- client/server contract changes are covered on both sides
- build/test commands are appropriate for the touched area

Update `plan.md` and add `Assessed: yes`.

## Phase 4: Implementation

Implement phase by phase.

- Use `apply_patch` for manual edits.
- Follow existing code style and local helper APIs.
- Keep database, queue, object storage, and observability checks inside Docker/MCP.
- For Postgres, remember the Docker port is host `15432` to container `5432`; in-container commands use `5432`.
- For Redis, use password `123456` when probing local dev containers.
- Do not edit unrelated files or reformat broad areas.
- Mark completed phases in `plan.md`.

## Phase 5: Verification

Use the full local build for any code change that may be deployed or runtime-tested. `deploy_services.bat` copies only from `build\bin\Release`, which is produced by `msvc2022-full`; do not use `build-verify-server` or `build-verify-client` for deployable verification.

Required build before deploy/runtime smoke:

```powershell
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
```

For unit tests, run them from the full build tree:

```powershell
ctest --preset msvc2022-full
```

For runtime verification, prefer existing scripts:

- `tools\scripts\preflight.ps1`
- `tools\scripts\status\deploy_services.bat`
- `tools\scripts\status\start-all-services.bat`
- `tools\scripts\status\stop-all-services.bat`
- `tools\scripts\test_register_login.ps1`
- `tools\scripts\test_login.ps1`
- `tools\scripts\full_flow_test.ps1`
- Python load tests under `tools\loadtest\python-loadtest`

If build fails with file locks or access denied, stop and tell the user which process/file is locking output.

## Phase 6: Review

Review the actual diff. Prioritize:

1. correctness and data consistency
2. threading/lifetime safety
3. API and schema compatibility
4. Docker/runtime config consistency
5. missing verification
6. unnecessary churn

Write `.ai/<project>/<letter>/review1.md`. Fix required issues and repeat up to three rounds.

## Phase 7: Completion

Before final response:

- Record verification results in `logs/phase-verify.result.md`.
- Confirm no unexpected Docker port/config drift.
- Summarize changed files, commands run, blockers, and remaining risk.
- Report elapsed time and the project name for follow-up work.
