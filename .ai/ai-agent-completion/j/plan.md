# Plan

Task summary: harden the AI Agent backend with regression coverage for AIServer JSON mapping and restart-aware Ollama recovery in AIOrchestrator.

Approach:

- Extract the AIServer response-mapping logic into a small pure helper so model list and KB list/delete behavior can be tested without networking or database dependencies.
- Add a dedicated AIServer gtest target covering the previously fragile field mappings.
- Add one-shot recovery logic inside the Ollama adapter that resets the cached HTTP client, probes readiness, and retries once when `/api/chat` fails in a way consistent with transient restart windows.
- Add Python unit tests that verify the recovery path instead of relying on runtime reproduction.

Files to modify or create:

- `apps/server/core/AIServer/AIServiceCore.cpp`
- `apps/server/core/AIServer/AIServiceJsonMapper.h`
- `apps/server/core/AIServer/AIServiceJsonMapper.cpp`
- `apps/server/core/AIServer/tests/CMakeLists.txt`
- `apps/server/core/AIServer/tests/main.cpp`
- `apps/server/core/AIServer/tests/AIServiceJsonMapperTest.cpp`
- `tests/CMakeLists.txt`
- `apps/server/core/AIOrchestrator/llm/ollama_llm.py`
- `apps/server/core/AIOrchestrator/tests/test_ollama_recovery.py`
- `.ai/ai-agent-completion/j/*`

Implementation phases:

- [x] Setup task artifact folder.
- [x] Gather context from existing AI Agent work, current source, and Docker logs.
- [x] Assess plan against actual test/build structure and running containers.
- [x] Extract AIServer JSON mapping helper and switch AIServiceCore to use it.
- [x] Add AIServer gtests for model list and KB list/delete mapping.
- [x] Add Ollama adapter recovery logic and Python regression tests.
- [x] Run targeted build/tests.
- [x] Review diff and record verification results.

Docker / MCP checks required:

- Keep `memochat-ai-orchestrator:8096` and `memochat-ollama:11434` port mappings unchanged.
- Use container logs as the source of truth for the observed 404 behavior.

Assessed: yes
