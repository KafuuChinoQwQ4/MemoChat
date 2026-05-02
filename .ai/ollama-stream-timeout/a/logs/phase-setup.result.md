User reported UI error: `发生错误: All LLM backends failed for streaming. Tried: ['ollama']` during a test on 2026-05-01.

Diagnosis start time: 2026-05-01T00:55:23.9952321-07:00.

Observed:
- Containers are running; `memochat-ai-orchestrator` healthy on 8096 and `memochat-ollama` healthy on 11434.
- Ollama has `qwen3:4b` installed.
- AIOrchestrator logs show `llm.backend_loaded backend=ollama url=http://memochat-ollama:11434`.
- Failure path: `ollama.stream_retry` then `llm.astream_error backend=ollama error=Ollama request failed:` then harness stream failure.
- Ollama logs show `/api/chat` returns 500 after exactly 2m0s and `aborting completion request due to client closing the connection`.
- Direct container probe from AIOrchestrator to Ollama with `hi`, stream=true, num_predict=8 took about 65.8s before first response line. The long user prompt can exceed the hard-coded 120s httpx read timeout before response headers.

Root cause: CPU-only Ollama/Qwen3 4B is slow enough that first token/header can exceed the 120s client read timeout for long prompts. The error message is also under-informative because `ReadTimeout` string is empty.
