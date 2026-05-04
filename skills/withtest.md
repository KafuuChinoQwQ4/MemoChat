---
description: Implement a MemoChat change, then run an iterative runtime test loop against Docker-backed local services.
---

# MemoChat With Test

Use when a change needs live validation beyond compilation, such as login, chat persistence, media storage, queues, observability, ops UI, or multi-service behavior.

## Stage 1: Implement

Follow `task.md`:

1. create `.ai/<project>/<letter>/`
2. gather context
3. plan
4. implement
5. build
6. review

## Stage 2: Runtime Test Loop

Maintain:

```text
.ai/<project>/<letter>/
  test1.md
  result1.md
  fix1.md
  screenshots/
  logs/
```

Repeat up to five iterations.

### Step A: Prepare Environment

Check Docker first:

```powershell
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

Start missing dependencies with the compose files under `infra/deploy/local` or the existing scripts. Keep the published ports unchanged.

### Step B: Deploy And Start Services

Use existing scripts:

```powershell
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
```

If files are locked, stop and report the locked process or executable. Do not fight Windows file locks by deleting build/runtime outputs repeatedly.

### Step C: Write Test Plan

Write `.ai/<project>/<letter>/test<N>.md` with:

- what behavior is being tested
- containers/services required
- scripts or commands to run
- data setup and cleanup
- expected logs/database/queue/object-store results
- screenshots needed for QML/Ops UI work
- success criteria

Prefer existing probes:

- `tools\scripts\test_register_login.ps1`
- `tools\scripts\test_login.ps1`
- `tools\scripts\test_login2.ps1`
- `tools\scripts\test_login3.ps1`
- `tools\scripts\full_flow_test.ps1`
- `python tools\loadtest\python-loadtest\py_loadtest.py --config tools\loadtest\python-loadtest\config.json --scenario all --total 20 --concurrency 5`
- service logs under `logs`, `artifacts`, and runtime service output files.

### Step D: Run Test

Run the planned commands. Capture:

- command line
- exit code
- relevant stdout/stderr
- Docker logs or Loki logs when useful
- database/queue/object-store verification queries
- screenshots for UI work

Write `.ai/<project>/<letter>/result<N>.md`.

### Step E: Assess

Choose one:

- `PASS`: behavior meets success criteria.
- `TEST_NEEDS_RERUN`: test setup/timing/assertion was wrong; adjust the test and rerun.
- `IMPLEMENTATION_NEEDS_FIX`: product code/config is wrong; write `.ai/<project>/<letter>/fix<N>.md`, fix only that issue, rebuild, redeploy, and restart the loop.

Do not leave debug-only instrumentation in production code unless the user explicitly wants it.

## Cleanup

When finished:

- stop locally started MemoChat services if the test started them
- leave Docker dependencies running unless the user asked to stop them
- record final status and any stateful data left behind
