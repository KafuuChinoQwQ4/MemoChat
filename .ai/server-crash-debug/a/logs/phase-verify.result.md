# Verification Result

Commands run:
- `cmake --preset msvc2022-server-verify`
- `cmake --build --preset msvc2022-server-verify`
- `tools\scripts\status\stop-all-services.bat`
- `tools\scripts\status\start-all-services.bat`
- `tools\scripts\test_register_login.ps1`

Outcome:
- Build: PASS
- Runtime stability: PASS for the observed window after restart; `GateServer`, `StatusServer`, and all four `ChatServer` instances remained running for more than 2 minutes.
- Smoke script: INCONCLUSIVE on the client path, because it still errors on the verify-code call even though the servers stay up.

Relevant evidence:
- No new Windows `APPCRASH` entries for `GateServer.exe` or `ChatServer.exe` in the recent observation window.
- Listening ports stayed bound for `8080`, `8082`, `8090-8093`, and `50052-50058`.

Follow-up on 2026-04-27:
- `start-all-services.bat` PowerShell windows were disappearing because `run-service-console.ps1` used PowerShell-native stderr handling for service processes.
- Windows Event Log showed repeated `powershell.exe` failures with `Management.Automation.PSInvalidOperation` / `ScriptBlock.GetContextFromTLS` before the fix.
- The wrapper was replaced with a synchronous runner and then adjusted to launch the service through `cmd /c "... 2>&1"` so native stderr is logged instead of being treated as a terminating PowerShell error.
- Verification commands:
  - PowerShell parser check for `tools\scripts\status\run-service-console.ps1`: PASS.
  - `git diff --check -- tools\scripts\status\run-service-console.ps1`: PASS.
  - `tools\scripts\status\stop-all-services.bat`: PASS.
  - `tools\scripts\status\start-all-services.bat`: PASS.
- Runtime result:
  - `GateServer.exe`, `StatusServer.exe`, four `ChatServer.exe` instances, and Node `VarifyServer` stayed running after startup.
  - Listening ports confirmed: `8080`, `8082`, `8083`, `8090-8093`, `50052`, `50055-50058`.
  - No new PowerShell crash events after the wrapper fix.
  - `GateServer` still logs non-fatal HTTP/3 credential warnings and continues on HTTP/1/HTTP/2.
