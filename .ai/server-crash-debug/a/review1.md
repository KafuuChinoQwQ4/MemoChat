# Review

Findings:
- No remaining crash-path issue in the touched code after the rebuild and post-start observation window.
- `GateServer` still reports HTTP/3 credential loading failure, but it falls back to HTTP/2 and keeps running. That is a runtime warning, not a crash.
- `test_register_login.ps1` still fails at the client side with connection/`NullReferenceException` style errors, but the server processes stayed alive and listening during the run.

Notes:
- The most important fix is the nghttp2 response-header lifetime bug in `NgHttp2Server.cpp`.
- The ChatServer shutdown hardening is defensive and reduces the chance of callback-after-stop crashes.
