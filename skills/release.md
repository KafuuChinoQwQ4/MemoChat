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

```powershell
git log --oneline --decorate -n 50
git diff --stat
```

If a previous tag exists, compare from that tag:

```powershell
git tag --sort=-v:refname
git log <tag>..HEAD --oneline
```

Write user-facing bullets. Skip noisy intermediate work, generated caches, local `.ai/` artifacts, and Docker data.

## Verify Docker Runtime Assumptions

Check fixed ports and container health:

```powershell
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
```

Do not change compose ports as part of release cleanup.

## Build/Test Matrix

Use the full local build before release/runtime deployment. The deployment script copies from `build\bin\Release`, so release validation must not depend on `build-verify-server` or `build-verify-client`.

```powershell
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
```

Run test presets when release scope needs automated tests:

```powershell
ctest --preset msvc2022-full
```

For runtime release confidence, use the existing service scripts and smoke tests:

```powershell
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
tools\scripts\test_register_login.ps1
tools\scripts\test_login.ps1
tools\scripts\full_flow_test.ps1
```

Stop on file-lock build/deploy errors and ask the user to close the locking process.

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
