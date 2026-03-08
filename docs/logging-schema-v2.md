# MemoChat Logging Schema v2

Each log line is newline-delimited JSON with these top-level fields:

- `ts`
- `level`
- `service`
- `service_instance`
- `env`
- `event`
- `message`
- `trace_id`
- `request_id`
- `span_id`
- `uid`
- `session_id`
- `module`
- `peer_service`
- `error_code`
- `error_type`
- `duration_ms`
- `attrs`

Rules:

- `attrs` stores non-core fields such as route, rpc name, Redis keys, or scenario tags.
- Sensitive values such as password, token, authorization, cookie, email, phone, and verify code must be redacted before write/export.
- `trace_id` and `request_id` are propagated across HTTP, gRPC metadata, and TCP JSON payloads.
- `span_id` identifies the current client/server/internal span used for trace export.
