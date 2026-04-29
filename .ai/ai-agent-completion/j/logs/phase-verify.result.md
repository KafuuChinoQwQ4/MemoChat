Verification summary:

- Python compile check:
  - `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m compileall llm tests`
  - Result: pass.
- Python unit tests:
  - `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest tests.test_ollama_recovery tests.test_harness_structure`
  - Result: pass, `7` tests total.
  - Recovery-path logs emitted during tests:
    - `ollama.chat_retry attempt=1 base_url=http://ollama body=404 page not found status_code=404`
    - `ollama.stream_retry attempt=1 base_url=http://ollama body=404 page not found status_code=404`
- C++ unit tests:
  - `cmake --preset msvc2022-tests`
  - `cmake --build --preset msvc2022-tests --target aiserver_gtest_json_mapper_test`
  - `ctest --preset msvc2022-tests`
  - Result: pass, `35/35` tests.
  - New AIServer mapper cases passed:
    - `AIServiceJsonMapperTest.PopulatesModelListAndExplicitDefaultModel`
    - `AIServiceJsonMapperTest.FallsBackToFirstModelWhenDefaultModelMissing`
    - `AIServiceJsonMapperTest.PopulatesKnowledgeBaseListFields`
    - `AIServiceJsonMapperTest.PopulatesKnowledgeBaseDeleteResponse`
- Server compile verification:
  - `cmake --preset msvc2022-server-verify`
  - `cmake --build --preset msvc2022-server-verify --target AIServer`
  - Result: pass.

Observations from runtime/log investigation:

- The intermittent failure exists in historical AIOrchestrator logs at `2026-04-28 16:08:23`:
  - `llm.astream_error backend=ollama error=Client error '404 Not Found' for url 'http://memochat-ollama:11434/api/chat'`
- Current container instances show `RestartCount=0` for both `memochat-ai-orchestrator` and `memochat-ollama`, so this phase focused on recovery hardening rather than reproducing a live restart race.
- No Docker port changes were made.

Environment notes:

- Initial `python -m unittest` with system Python failed because `structlog` was not installed there; verification was rerun with the project venv under `apps/server/core/AIOrchestrator/.venv`.
- One intermediate C++ build failed in the new helper due to `JsonValue` vs `JsonMemberRef` overload assumptions; fixed by switching the helper to explicit object/array traversal.
