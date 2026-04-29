# Verification

Commands run:

- `cmake --preset msvc2022-server-verify`: PASS. Configure completed; only existing CMake deprecation/dev warnings were reported.
- `cmake --build --preset msvc2022-server-verify`: PASS. `ChatServer.exe` linked successfully.
- `docker exec memochat-postgres psql ... EXPLAIN ...`: PASS after setting `search_path=memo,public`; fixed unread query parses and plans against `memo.chat_private_msg` and `memo.chat_private_read_state`.
- `docker exec memochat-postgres psql ... old_count/fixed_count`: PASS. For sample unread dialog `owner_uid=1059, peer_uid=1060`, old logic returned 0 rows while fixed logic returned 1 row.
- `cmake --build --preset msvc2022-full --target ChatServer`: PASS. Main build output `build/bin/Release/ChatServer.exe` updated for runtime deployment.
- `tools\scripts\status\deploy_services.bat`: PASS. Runtime services deployed to `infra\Memo_ops\runtime\services`.
- `tools\scripts\status\start-all-services.bat`: PASS by port/process check. Gate 8080/8084, Status 50052/50382, Chat 8090/8091/8092/8093/8094/8097 and Chat RPC 50055/50056/50057/50058/50059/50381 were listening. Varify Node processes were listening on 50051 and 50383.
- `curl.exe -i -s -X POST http://127.0.0.1:8080/user_login ...`: PASS for HTTP/service smoke. Response was HTTP 200 with business error `1009` for invalid test credentials.

Notes:

- `tools\scripts\test_login.ps1` is inconclusive: it prints `Object reference not set to an instance of an object.` and exits 0. Direct `curl.exe` verified the HTTP route instead.
- No schema changes were made.
