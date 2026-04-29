Review result: passed.

Checked:
- API key is not included in `/models` or `/ai/model/list` responses.
- Runtime provider ids are normalized with an `api-` prefix and are used as `model_type`.
- Chat resolution will route non-built-in provider ids through the custom OpenAI-compatible client.
- The model popup refreshes the list after successful registration.

Residual risk:
- Only OpenAI-compatible providers are supported by the UI registration path. Native Anthropic-style Claude registration still needs a dedicated adapter form if desired.
