# Phase Verify

Commands run from `apps/server/core/AIOrchestrator`:

```powershell
.venv\Scripts\python.exe -m py_compile harness\interop\service.py harness\contracts.py harness\runtime\container.py api\agent_router.py schemas\api.py tests\test_harness_structure.py
.venv\Scripts\python.exe -m unittest tests.test_harness_structure
git diff --check
```

Results:
- `py_compile`: passed.
- `unittest`: passed, 28 tests.
- `git diff --check`: passed with existing LF-to-CRLF normalization warnings only.

Docker/MCP:
- Not used. This stage only touched Python harness/API/test code and did not change infrastructure, ports, or persisted schemas.
