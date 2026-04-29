Review result: passed after one small follow-up.

Findings addressed:
- `config.yaml` had dropped the existing `qwen2.5:3b` and `qwen2.5-coder:3b` Ollama entries while adding `supports_thinking`. Restored both with `supports_thinking: false`.
- `AIServiceJsonMapperTest` did not cover the new `supports_thinking` model field. Added assertions for model list and default model propagation.

Residual risks:
- External providers are configured disabled-by-default and require env vars/API keys plus runtime enabling before live calls can be validated.
- Provider-specific reasoning parameters differ by vendor; OpenAI-compatible reasoning extraction currently handles common `reasoning_content`/`reasoning` fields and configurable request flags.
