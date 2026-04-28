# Review

Outcome: pass.

Findings:
- Fixed a runtime script issue caused by mixed CRLF/LF line endings in `start-all-services.bat` and `stop-all-services.bat`. `cmd.exe` was dropping leading characters from later lines, which prevented the C++ launch section from running reliably.
- Fixed `stop-all-services.bat` so its runner-window cleanup does not kill the PowerShell process currently doing the cleanup.
- Fixed Node Varify startup environment assignment to avoid nested quote parsing and trailing spaces.
- Added deploy required-file checks for `VarifyServer1` and `VarifyServer2`.

Verification:
- `deploy_services.bat` deployed `StatusServer1`, `StatusServer2`, `VarifyServer1`, and `VarifyServer2`.
- Runtime listeners are present for 2 Gate, 6 Chat, 2 Status, and 2 Varify instances.
- `GateServer1` runtime config points to Varify `50051` and Status `50052`.
- `GateServer2` runtime config points to Varify `50383` and Status `50382`.
- `git diff --check` passed for touched files.
- PowerShell parser checks passed for touched PS scripts.
- Docker compose config check passed.
