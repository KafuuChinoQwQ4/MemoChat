@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"
set "STATUS_CONFIG=%ROOT%\server\StatusServer\config.ini"
set "PID_ROOT=%ROOT%\Memo_ops\artifacts\runtime\pids"

echo [INFO] Stopping MemoChat test services...

powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\Memo_ops\scripts\stop_ops_platform.ps1" -PidRoot "%PID_ROOT%" >nul 2>nul

if exist "%PID_ROOT%" (
  for %%F in ("%PID_ROOT%\*.pid") do (
    if exist "%%~fF" (
      for /f "usebackq delims=" %%P in ("%%~fF") do (
        powershell -NoProfile -ExecutionPolicy Bypass -Command "Stop-Process -Id %%P -Force -ErrorAction SilentlyContinue" >nul 2>nul
      )
      del /Q "%%~fF" >nul 2>nul
    )
  )
)

for %%P in (GateServer.exe StatusServer.exe ChatServer.exe MemoChatQml.exe MemoOpsQml.exe GateServerDrogon.exe) do (
  taskkill /F /IM %%P >nul 2>nul
)

for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$root='%ROOT%'.ToLowerInvariant();" ^
  "Get-CimInstance Win32_Process -Filter \"Name='node.exe'\" -ErrorAction SilentlyContinue | Where-Object {" ^
  "  $_.CommandLine -and $_.CommandLine.ToLowerInvariant().Contains(($root + '\\server\\varifyserver\\server.js'))" ^
  "} | ForEach-Object { $_.ProcessId }"`) do (
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Stop-Process -Id %%P -Force -ErrorAction SilentlyContinue" >nul 2>nul
)

for %%P in (50051 50052 8080 8081 8443 8090 8091 8092 8093 8190 8191 8192 8193 50055 50056 50057 50058) do (
  for /f "tokens=5" %%I in ('netstat -ano -p TCP ^| findstr /R /C:":%%P .*LISTENING"') do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Stop-Process -Id %%I -Force -ErrorAction SilentlyContinue" >nul 2>nul
  )
  for /f "tokens=2" %%I in ('netstat -ano -p UDP ^| findstr /R /C:":%%P "') do (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Stop-Process -Id %%I -Force -ErrorAction SilentlyContinue" >nul 2>nul
  )
)

echo [DONE] MemoChat services stopped.
exit /b 0
