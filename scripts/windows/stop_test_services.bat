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

for %%P in (GateServer.exe StatusServer.exe ChatServer.exe MemoChatQml.exe MemoOpsQml.exe) do (
  taskkill /F /IM %%P >nul 2>nul
)

echo [DONE] MemoChat services stopped.
exit /b 0
