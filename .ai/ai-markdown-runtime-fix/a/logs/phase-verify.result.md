Verification:

- `.venv\Scripts\python.exe -m py_compile main.py api\agent_router.py`: passed.
- `cmake --preset msvc2022-client-verify`: passed.
- `cmake --build --preset msvc2022-client-verify`: passed.
- `.venv\Scripts\python.exe -m unittest tests.test_harness_structure`: passed, 28 tests OK.
- Runtime probe:
  - `GET http://127.0.0.1:8096/health`: ok.
  - `GET http://127.0.0.1:8096/agent/memory?uid=1&limit=5`: 404 in the currently running container.
  - `GET http://127.0.0.1:8096/agent/tasks?uid=1&limit=5`: 404 in the currently running container.

Runtime conclusion:
- The running AIOrchestrator container is older than local code and must be rebuilt/restarted before Memory/Task panes stop showing service errors.
