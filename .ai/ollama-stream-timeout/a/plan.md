# Plan

Summary: Make Ollama stream timeout configurable/longer and improve timeout diagnostics.

Approach:
- Add `timeout_sec` to `OllamaLLMConfig`, default 300.
- Set `llm.ollama.timeout_sec: 300` in config.yaml.
- Pass timeout from `LLMManager` into `OllamaLLM`.
- Use the timeout in `OllamaLLM._get_client` and log exception type for request errors.
- Improve manager stream error logging with exception type.
- Verify with py_compile, targeted Ollama tests, and container direct slow probe.

Checklist:
- [x] Code/config edits
- [x] Tests/compile
- [x] Runtime/container verification
- [x] Review

Assessed: yes. No port or compose mapping change was needed. Runtime config is mounted from `apps/server/core/AIOrchestrator/config.yaml`; AIOrchestrator was rebuilt/recreated through its existing compose file to apply code/config.
