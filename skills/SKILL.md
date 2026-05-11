---
name: memochat-task-think
description: Orchestrate a multi-phase implementation workflow for MemoChat-Qml-Drogon. Use when a task needs structured context gathering, planning, implementation, Docker-backed data checks, CMake build verification, review, and persistent artifacts under the local .ai project folders.
---

# MemoChat Task Pipeline

Use this skill to drive non-trivial changes in `/root/code/MemoChat-Qml-Drogon-linux` while keeping the main thread thin and preserving useful artifacts. Treat `D:\MemoChat-Qml-Drogon` as the legacy Windows checkout unless the user explicitly asks for Windows-side work.

For implementation work, `skills/parallel-agents.md` is the mandatory default execution mode. The main thread acts as the Controller: it owns architecture, plan, shared contracts, worker dispatch, integration, review, and final acceptance. After the Controller has enough context and freezes the first shared contract, it must dispatch safe disjoint worker lanes immediately to accelerate delivery. Local-only execution is an exception, allowed only when the active tool/policy environment forbids spawning workers, the user explicitly asks for single-agent work, the task is genuinely tiny and has no useful test/review lane, the task is strictly sequential, or no safe split exists. Record the exact exception reason in `plan.md` before continuing local-only.

## Project Rules

- Treat Arch Linux in WSL bash as the default shell for build, deploy, runtime, and verification work.
- Use Windows PowerShell only for legacy Windows client checks, Docker Desktop migration/backup operations, or explicit Windows checkout work.
- Keep all infrastructure dependencies in Docker. Check containers with `docker ps`; inspect databases and queues through Docker or the configured MCP tools, not local host installs.
- Arch native Docker is the default Docker runtime. Source `/root/.memochat-linux-env`, which unsets `DOCKER_HOST` and uses `/var/run/docker.sock`.
- Keep container ports stable. Do not change compose port mappings unless the task explicitly asks for it.
- Prefer downloading Linux caches, models, packages, vcpkg artifacts, Qt artifacts, and generated large files under `/data`.
- Use `/data/docker-data/memochat` for local Docker bind data. Use `D:` only for legacy Docker Desktop backups, Windows artifacts, or explicit Windows-side checks.
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

1. Record start time with `date`.
2. Detect follow-up work:
   - If the first token of the request matches `.ai/<token>/about.md`, use that project.
   - Otherwise create a short kebab-case project name.
3. Pick the next task letter (`a`, `b`, ...), create directories, and write the task request to `logs/phase-setup.result.md`.
4. Capture any screenshot or attachment summary into `context.md` or a log file before delegation.
5. For code changes, read `skills/parallel-agents.md` and open Controller-led concurrency by default. Dispatch workers as soon as a safe contract and ownership split exist. If workers are not dispatched, record the blocker or exception reason in `plan.md` before implementation.

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
- concurrency decision:
  - Controller duties
  - worker lanes to spawn by default
  - lane write ownership
  - exact reason if worker dispatch is blocked, unsafe, or intentionally skipped

Keep phases small enough that one agent or one focused pass can complete each phase.

When parallel lanes are useful, the Controller freezes the first shared contract before workers edit files.

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
- For parallel work, keep the Controller focused on orchestration, contract updates, integration, and acceptance while workers own disjoint file scopes.
- Follow existing code style and local helper APIs.
- Keep database, queue, object storage, and observability checks inside Docker/MCP.
- For Postgres, remember the Docker port is host `15432` to container `5432`; in-container commands use `5432`.
- For Redis, use password `123456` when probing local dev containers.
- Do not edit unrelated files or reformat broad areas.
- Mark completed phases in `plan.md`.

## Phase 5: Verification

Use the Linux GCC16 presets for deployable Arch/WSL runtime verification. `deploy_services.sh` copies from `build-linux-server-gcc16/bin` by default. For client-only Linux work use `linux-client-gcc16`; for cross-stack Linux checks use `linux-full-gcc16`.

Required build before deploy/runtime smoke:

```bash
source /root/.memochat-linux-env
cmake --preset linux-server-gcc16
cmake --build --preset linux-server-gcc16 --parallel 12
```

For unit tests, run them from the full build tree:

```bash
ctest --preset linux-server-gcc16 --output-on-failure
```

For runtime verification, prefer existing scripts:

- `tools/scripts/status/deploy_services.sh`
- `tools/scripts/status/start-all-services.sh`
- `tools/scripts/status/stop-all-services.sh`
- Legacy Windows probes, when running from Windows: `tools/scripts/test_register_login.ps1`, `tools/scripts/test_login.ps1`, `tools/scripts/full_flow_test.ps1`
- Python load tests under `tools/loadtest/python-loadtest`

The `.bat`/`.ps1` scripts remain legacy Windows equivalents. If Docker is unavailable in Arch, start or repair the Arch Docker daemon before runtime smoke.

## Phase 6: Review

Review the actual diff. Prioritize:

1. correctness and data consistency
2. threading/lifetime safety
3. API and schema compatibility
4. Docker/runtime config consistency
5. missing verification
6. unnecessary churn

Write `.ai/<project>/<letter>/review1.md`. Fix required issues and repeat up to three rounds.

For parallel work, the Controller must read every used `logs/parallel-*.result.md`, inspect the actual diff, confirm contracts match final code, and only then accept the task.

## Phase 7: Completion

Before final response:

- Record verification results in `logs/phase-verify.result.md`.
- Record the concurrency outcome: worker lanes used, local-only reason, or any blocked/deferred lane.
- Confirm no unexpected Docker port/config drift.
- Summarize changed files, commands run, blockers, and remaining risk.
- Report elapsed time and the project name for follow-up work.
