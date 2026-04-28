# MemoChat Phase Prompts

Use these templates for delegated agents or as same-session checklists. Replace `<TASK>`, `<PROJECT>`, `<LETTER>`, and `<REPO_ROOT>`.

## Shared Rules

- Work in `D:\MemoChat-Qml-Drogon`.
- Infrastructure dependencies must run in Docker. Use Docker or MCP tools for Redis, Postgres, MongoDB, Neo4j, Qdrant, Redpanda, RabbitMQ, MinIO, Prometheus, Loki, Tempo, Grafana, InfluxDB, and cAdvisor.
- Do not change stable Docker ports unless the task explicitly asks for it.
- Prefer D: for downloads, caches, large generated files, and Docker data.
- Do not revert user changes.
- Keep `.ai/` artifacts out of commits unless explicitly requested.
- Use PowerShell commands in examples.

## Standard Progress Contract

For delegated phases, create `.ai/<PROJECT>/<LETTER>/logs/phase-<name>.progress.md` early:

```text
Heartbeat: <N>
Current step:
Files read/edited:
Findings:
Next checkpoint:
Blocker: none
```

Keep it small. Update it at natural milestones.

## Standard Reply

Reply in 8 lines or fewer:

```text
STATUS: DONE|BLOCKED|APPROVED|NEEDS_CHANGES
ARTIFACTS: <paths>
TOUCHED: <repo paths or none>
BLOCKER: <none or one short line>
```

## Phase 1: Context

```text
You are gathering context for MemoChat-Qml-Drogon.

TASK:
<TASK>

Write:
- .ai/<PROJECT>/about.md
- .ai/<PROJECT>/<LETTER>/context.md

Steps:
1. Read the repository structure and relevant README/config files.
2. Inspect source files related to the task.
3. Inspect Docker compose/config/migrations/scripts when runtime state is relevant.
4. Query Docker or MCP only when it helps verify real database, queue, object-store, or observability state.
5. Identify similar implementations and existing helper APIs.

context.md must include:
- task restatement
- relevant files and why they matter
- functions/classes/config keys/schemas involved
- Docker containers and fixed ports involved
- MCP/Docker checks performed
- build/test/runtime commands recommended
- risks and unknowns

about.md must describe the project as a completed design, not as a TODO list.
```

## Phase 2: Plan

```text
You are planning a MemoChat implementation.

Read:
- .ai/<PROJECT>/<LETTER>/context.md
- source/config files referenced by context.md

Write .ai/<PROJECT>/<LETTER>/plan.md with:

## Task
## Approach
## Files to Modify
## Files to Create
## Docker / Data Impact
## Implementation Phases
## Verification
## Status

Make every phase concrete: exact files, functions, configs, scripts, migrations, and verification commands.
Choose the narrowest build/test target that proves the change.
```

## Phase 3: Assess Plan

```text
You are assessing a MemoChat implementation plan.

Read:
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md
- referenced source/config files

Check:
1. paths and symbols exist
2. module boundaries are right
3. client/server contracts are synchronized
4. database migrations/init scripts are covered
5. Docker ports and container assumptions are correct
6. verification is sufficient and not excessive

Update plan.md in place.
Add `Phases: <N>` under Status and `Assessed: yes` at the end.
```

## Phase 4: Implement

```text
You are implementing Phase <N> for MemoChat.

Read:
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md

Implement only this phase:
<PHASE_STEPS>

Rules:
- Use existing patterns and helpers.
- Keep Docker/MCP assumptions consistent with the plan.
- Do not edit unrelated files.
- Do not modify .ai files except plan status.
- Mark this phase done in plan.md when complete.
```

## Phase 5: Verify

```text
You are verifying a MemoChat change.

Read:
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md

Run the narrowest relevant verification:

Server:
cmake --preset msvc2022-server-verify
cmake --build --preset msvc2022-server-verify

Client:
cmake --preset msvc2022-client-verify
cmake --build --preset msvc2022-client-verify

Tests:
cmake --preset msvc2022-tests
cmake --build --preset msvc2022-tests
ctest --preset msvc2022-tests

Runtime smoke, when needed:
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
tools\scripts\test_register_login.ps1
tools\scripts\test_login.ps1
tools\scripts\full_flow_test.ps1

If a file-lock or access-denied build/deploy error appears, stop and report the locked file/process.
Write the result to .ai/<PROJECT>/<LETTER>/logs/phase-verify.result.md.
```

## Phase 6: Review

```text
You are reviewing the MemoChat diff.

Read:
- .ai/<PROJECT>/<LETTER>/context.md
- .ai/<PROJECT>/<LETTER>/plan.md
- git diff

Write .ai/<PROJECT>/<LETTER>/review<R>.md.

Prioritize:
1. correctness and persistence consistency
2. race/lifetime/thread safety
3. schema/config/API compatibility
4. Docker/runtime impact
5. missing verification
6. unnecessary churn

Verdict must be `APPROVED` or `NEEDS_CHANGES`.
For NEEDS_CHANGES, include concrete file-level fixes.
```

## Phase 7: Finalize

```text
Finalize in the main session.

Check:
- plan status
- verification log
- review verdict
- git status
- no unexpected Docker port/config drift

Report:
- files changed
- verification commands and results
- Docker/MCP checks
- blockers or residual risk
- .ai project name for follow-up
```
