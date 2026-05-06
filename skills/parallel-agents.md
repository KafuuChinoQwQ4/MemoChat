---
name: parallel-agents
description: Default MemoChat concurrency workflow. Use for every implementation task to decide and run a Controller-led set of parallel agents whenever any useful work can be split safely.
---

# MemoChat Parallel Agents

Use this skill for every MemoChat implementation task after reading `skills/SKILL.md` and the relevant focused skill. The default posture is **parallel by default**: after the Controller gathers enough context and freezes the first shared contract, dispatch safe disjoint workers immediately when the active tool/policy environment permits it.

Tiny one-line fixes may stay single-agent, but the decision must be explicit in `plan.md`. Any task with more than one of context gathering, architecture, backend, frontend, database, tests, runtime verification, docs, or review should use the Controller-led parallel model. If worker dispatch is blocked by active tool or platform policy, record `Concurrency decision: parallel blocked by active tool/policy; Controller continued local-only` and keep the rest of the workflow intact.

## Non-Negotiable Shape

Every parallel task has one **Controller agent**.

The Controller is mandatory and owns:

- architecture design and non-goals
- `.ai/<project>/<letter>/context.md`
- `.ai/<project>/<letter>/plan.md`
- lane decomposition and write ownership
- shared contracts, DTO/API/QML property names, migrations, feature flags, and config boundaries
- prompt construction for worker agents
- integration order and conflict policy
- final diff review and overall acceptance
- final verification decision and residual risk summary

The Controller may implement small glue changes only when that unblocks integration. It should not disappear into broad product code while workers are running.

## Default Agent Topology

Use one Controller plus up to five worker lanes. Fewer lanes are fine, but Controller must always exist for concurrent work. For normal implementation tasks, the minimum default topology is Controller + Tests Worker + at least one implementation or investigation lane when a safe disjoint write/read scope exists.

For any non-trivial implementation task, include a dedicated **Tests Worker** by default. This lane keeps testing alive while implementation proceeds and continues after integration: it expands unit, smoke, regression, and boundary coverage for the actual behavior, including invalid inputs, empty states, repeated operations, lifecycle transitions, concurrency/retry cases, persistence round trips, and UI/API contract mismatches. Skip it only when the task is truly tiny or no test surface exists, and record the reason in `plan.md`.

| Role | Harness layer | Owns | Typical write scope |
| --- | --- | --- | --- |
| Controller | orchestration + acceptance | architecture, plan, contracts, prompts, merge order, review, final acceptance | `.ai/<project>/<letter>/**`, small integration glue |
| Backend Worker | execution | server logic, API routes, schemas, migrations, configs | `apps/server/core/**`, `apps/server/config/**`, `apps/server/migrations/**` |
| Frontend Worker | execution | QML/client controller UX and resource wiring | `apps/client/desktop/**`, `infra/Memo_ops/client/**` |
| Data/AI Worker | execution | AIOrchestrator, RAG, model/tool contracts, Docker-backed AI dependencies | `apps/server/core/AIOrchestrator/**`, AI/RAG configs |
| Tests Worker | feedback + quality gate | unit tests, smoke probes, load scripts, boundary/regression coverage, verification artifacts | `tests/**`, `apps/**/tests/**`, `tools/scripts/**`, `tools/loadtest/**` |
| Integration Worker | runtime/review | compile fixes, cross-lane wiring, runtime smoke, docs | narrow files approved by Controller |

When a task has different natural slices, rename lanes by ownership, not by ego. Examples: `Schema Worker`, `QML Layout Worker`, `Gateway Proxy Worker`, `Observability Worker`.

## Required Dispatch Pattern

1. Controller starts first in the main thread.
2. Controller reads enough code and Docker/MCP state to create a useful context pack.
3. Controller writes or updates:
   - `.ai/<project>/<letter>/context.md`
   - `.ai/<project>/<letter>/plan.md`
   - `.ai/<project>/<letter>/contracts.md` when contracts matter
4. Controller freezes the first contract slice before any worker changes files.
5. Controller dispatches independent workers with self-contained prompts by default when safe lanes exist and spawning is permitted.
6. Controller dispatches a Tests Worker for non-trivial implementation so testing continues in parallel with product changes. Skip only when there is no meaningful test surface, the task is genuinely tiny, or worker dispatch is blocked by active policy; record the reason in `plan.md`.
7. Controller continues useful non-overlapping work locally while workers run.
8. Worker results return through their result files and final messages.
9. Controller integrates implementation lanes, then gives the Tests Worker the final behavior/diff when additional post-integration coverage or regression checks are needed.
10. Controller inspects actual diffs, integrates, verifies, reviews, and accepts or sends targeted follow-up work.

Do not spawn workers for the immediate blocking task. If the next local step depends on one answer, Controller handles that answer locally first, then dispatches parallel lanes.

## Parallelization Heuristics

Dispatch parallel workers by default when:

- backend and frontend can be defined through a stable API/QML contract
- tests can be written against an agreed behavior while implementation proceeds
- a Tests Worker can expand boundary cases, lifecycle cases, repeated-operation checks, and regression coverage against a stable contract
- one lane can inspect Docker/MCP/data state while another edits code
- documentation or `.ai` artifact updates can happen while product code is being implemented
- review/runtime smoke can start from partial lane output without inventing new scope

Keep work local only when:

- the task is a one-line or single-symbol edit
- the active tool/policy environment forbids spawning workers for the current request
- the user explicitly asks for single-agent/local-only execution
- requirements are too unclear to freeze any contract
- two workers would need to edit the same file at the same time
- a migration or port/config change needs a user decision first
- the work is sequential by nature and parallel agents would only add merge risk
- no meaningful test, smoke, or static-check surface exists for the requested change

When keeping work local, add a short note to `plan.md`:

```text
Concurrency decision: local-only because <reason>.
```

## Worker Prompt Contract

Every worker prompt must include:

- task goal in one paragraph
- required skill files already considered by Controller
- current `.ai/<project>/<letter>/context.md`, `plan.md`, and `contracts.md` paths
- exact ownership: files/directories the worker may edit
- files/directories the worker must not touch
- expected build/test commands for its lane
- Docker/MCP rules and stable port warning
- instruction that other agents may edit the repo concurrently
- instruction not to revert user or other-agent changes
- required result file path

Tests Worker prompts must also include the expected behavior matrix and known edge cases. Ask the lane to add or update tests before running them, then report untested risks explicitly. For stateful flows, it should cover create/read/update/delete where relevant, repeated start/stop or restart paths, fixed-config invariants, randomization boundaries, empty and maximum supported collections, invalid payloads, persistence reloads, and API/UI mismatch scenarios.

Use `skills/PROMPTS.md` only when constructing larger reusable prompts; otherwise a concise self-contained prompt is enough.

## Worker Output Contract

Each worker writes:

- `.ai/<project>/<letter>/logs/parallel-<lane>.progress.md`
- `.ai/<project>/<letter>/logs/parallel-<lane>.result.md`

Each result must include:

```text
STATUS: DONE|BLOCKED|NEEDS_INTEGRATION|NEEDS_DECISION
ROLE: Controller|Backend|Frontend|DataAI|Tests|Integration|Custom
OWNERSHIP: files/directories owned
CHANGED: paths changed, or none
CONTRACTS: APIs/schema/QML properties/events changed
VERIFY: commands run and result
RISKS: remaining issues
NEXT: exact integration step
```

Controller may reject a worker result that lacks enough evidence to integrate safely.

## Controller Acceptance Gates

Controller cannot mark the task done until it has:

1. Read all used `parallel-*.result.md` files.
2. Checked `git status` and relevant `git diff`.
3. Confirmed worker write scopes did not collide unexpectedly.
4. Confirmed shared contracts match final code.
5. Confirmed the Tests Worker either ran and expanded meaningful coverage, or `plan.md` records why a separate testing lane was not useful.
6. Run the narrowest meaningful verification, plus `msvc2022-full` when deploy/runtime behavior is touched.
7. Written `.ai/<project>/<letter>/review1.md`.
8. Recorded verification in `.ai/<project>/<letter>/logs/phase-verify.result.md`.
9. Updated `plan.md` statuses and the concurrency decision.

If any gate fails, Controller either fixes a narrow issue locally, sends one targeted follow-up to the owning worker, or asks the user when the decision is product-level or destructive.

## Lane Ownership Rules

- Keep write ownership disjoint.
- If two lanes need the same file, Controller assigns one owner. The other lane writes a note or patch suggestion in its result file.
- Workers must not broaden scope to "while I am here" refactors.
- Workers must not update shared contracts unilaterally. They propose changes to Controller.
- Workers must not run destructive git commands.
- Workers must not change Docker published ports unless the user explicitly requested it and Controller approved it.

## Docker And Runtime Rules

Infrastructure truth comes from Docker and configured MCP tools.

Before a lane relies on a datastore, queue, object store, observability service, or AI/RAG dependency, it should inspect the relevant container or MCP state and record the command/query in context or result logs.

Stable ports must stay stable. If a proposed plan requires changing a published port, stop and ask the user.

## Recommended Lane Splits

For API + UI work:

- Controller: endpoint/QML contract, ownership, final review
- Backend Worker: server/API/schema
- Frontend Worker: QML/client controller
- Tests Worker: API smoke and UI/static checks
- Integration Worker: build/runtime verification

For AI/RAG work:

- Controller: architecture, model/tool contract, data flow
- Data/AI Worker: AIOrchestrator/RAG/tool changes
- Backend Worker: Gate/AIServer proxy or C++ contract changes
- Tests Worker: offline tests and Docker-backed probes
- Integration Worker: runtime smoke and observability checks

For database/migration work:

- Controller: schema design, migration order, rollback/compatibility risk
- Backend Worker: DAO/service changes
- Data Worker: migration/init scripts and Docker verification
- Tests Worker: idempotency and query probes
- Integration Worker: build and runtime smoke

For QML-heavy work:

- Controller: UX contract and client/server data requirements
- Frontend Worker: QML layout/components/resources
- Backend/Data Worker only if API data changes
- Tests Worker: `qmllint`, resource checks, screenshots when available
- Integration Worker: client build and smoke

## Reusable Template

Use `.ai/parallel-harness-template/` as the default scaffold for repeated parallel work:

- `about.md` explains the Controller-led concurrency model.
- `prompt.md` is the base dispatch prompt.
- `tasks.json` contains Controller plus worker lanes.

When using the template on a real task, copy or adapt it into `.ai/<project>/<letter>/` and replace placeholders with actual contracts, ownership, and verification commands.

## Completion Checklist

Before final response:

- Controller result exists or Controller duties are recorded in `plan.md`
- all used worker result files exist
- contracts and final code agree
- plan status reflects completed and deferred work
- verification commands and outcomes are recorded
- no unexpected Docker port/config drift
- final answer names changed files, verification, blockers, and `.ai` project path
