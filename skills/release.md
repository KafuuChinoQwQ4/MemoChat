---
description: Prepare a MemoChat release summary, verify the tree, run selected builds/tests, and create a release commit or tag when requested.
---

# MemoChat Release

Use when preparing a release or release candidate for this repository.

## Preconditions

1. Run `git status --short`.
2. If unrelated uncommitted changes exist, stop and ask how to proceed.
3. Confirm which scope is being released:
   - server
   - client
   - ops
   - full stack
4. Confirm whether the user wants a commit, a tag, both, or only a release note.

## Gather Changes

Use:

```bash
git log --oneline --decorate -n 50
git diff --stat
```

If a previous tag exists, compare from that tag:

```bash
git tag --sort=-v:refname
git log <tag>..HEAD --oneline
```

Write user-facing bullets. Skip noisy intermediate work, generated caches, local `.ai/` artifacts, and Docker data.

## Verify Docker Runtime Assumptions

Check fixed ports and container health:

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
```

Do not change compose ports as part of release cleanup.

## Build/Test Matrix

Use the Linux GCC16 build before Arch/WSL release/runtime deployment. The deployment script copies from `build-linux-server-gcc16/bin`.

```bash
source /root/.memochat-linux-env
cmake --preset linux-server-gcc16
cmake --build --preset linux-server-gcc16 --parallel 12
```

Run test presets when release scope needs automated tests:

```bash
ctest --preset linux-server-gcc16 --output-on-failure
```

For runtime release confidence, use the existing service scripts and smoke tests:

```bash
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

Legacy Windows smoke probes may still be run from Windows when needed:

```powershell
tools/scripts/test_register_login.ps1
tools/scripts/test_login.ps1
tools/scripts/full_flow_test.ps1
```

Stop on port conflicts or Docker dependency failures and report the owning process/container.

## Release Note Shape

Use concise bullets grouped by:

- Added
- Changed
- Fixed
- Ops / Infrastructure

Keep implementation detail out unless it affects deployment or operation.

## Commit/Tag

Only commit or tag when the user explicitly approves the final release text.

Before committing:

- ensure `.ai/` artifacts are not staged unless explicitly requested
- ensure ignored Docker data and local caches are not staged
- include verification commands and outcomes in the final message
