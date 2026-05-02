# Review 1

Reviewed changed backend and QML trace UI files.

Findings fixed during review:
- Tool-policy blocked calls initially lacked structured `call` metadata. Added `_blocked_call` so confirmation-required, invalid-argument, and missing-tool cases expose masked request payload, status, reason, and duration.
- Prompt/output preview initially only masked structured keys. Added text redaction for common `api_key`, `token`, `password`, `authorization`, and `sk-*` patterns before trace previews are persisted.

Residual risk:
- QML detail rendering uses plain `TextEdit` blocks for selectable JSON. It builds and is conservative, but actual visual density should be checked during a live run.
- Historical persisted traces keep old metadata shape; the UI tolerates missing fields.
