# LLM Routing Layer

Purpose:
- List local and external model endpoints.
- Resolve a model endpoint from backend, model name, or deployment preference.
- Provide non-streaming and streaming model calls.

Primary files:
- `service.py`

Coupling rule:
- Do not embed UI or API response logic here. Return `LLMResponse` or `LLMStreamChunk` only.
