# Test 1

## Behavior

Verify that AIOrchestrator no longer leaves the UI with no visible answer when a complex Chinese algorithm prompt is sent through `/chat/stream`.

## Scope

Included:

- Backend SSE stream from `memochat-ai-orchestrator`.
- Ollama-backed local model path.
- UTF-8 Chinese request body.
- Visible SSE chunks.

Excluded:

- Full QML UI screenshot verification.
- Semantic quality of the algorithm answer.
- Waiting for the local CPU model to finish a very long answer.

## Commands

```powershell
python -m compileall apps\server\core\AIOrchestrator\harness\orchestration\agent_service.py apps\server\core\AIOrchestrator\llm\ollama_llm.py apps\server\core\AIOrchestrator\tests\test_harness_structure.py
docker exec memochat-ai-orchestrator python -m unittest tests.test_harness_structure -q
curl.exe -N -s --max-time 90 -H "Content-Type: application/json; charset=utf-8" --data-binary "@<utf8-json-body>" http://127.0.0.1:8096/chat/stream
```

## Success Criteria

- Compile check succeeds.
- Container unit test passes.
- Runtime SSE returns one or more non-empty Chinese `chunk` values.
- The stream does not immediately return only the model-empty message.
