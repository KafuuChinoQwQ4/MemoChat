# AI Agent Runtime Polish

MemoChat AI Agent runtime handles empty model output without presenting it as a safety block, keeps tool execution compatible with current dependencies, renders assistant code blocks cleanly, persists registered API provider model lists across app/container restarts, and filters local Ollama models to only those actually installed.
