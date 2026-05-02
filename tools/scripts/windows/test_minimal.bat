@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"

set "BACKGROUND=1"
set "WITH_OPS=1"
set "WITH_CLIENT=1"
set "ENABLE_QUIC=0"
for %%A in (%*) do (
  if /I "%%~A"=="--foreground" set "BACKGROUND=0"
  if /I "%%~A"=="--no-client" set "WITH_CLIENT=0"
  if /I "%%~A"=="--skip-ops" set "WITH_OPS=0"
  if /I "%%~A"=="--enable-quic" set "ENABLE_QUIC=1"
)

set "SERVER_BIN_PRIMARY=%ROOT%\build-vcpkg-server\bin\Release"
set "SERVER_BIN_FALLBACK=%ROOT%\build\bin\Release"
set "BUILD_BIN=%SERVER_BIN_PRIMARY%"
set "GATE_EXE="
set "STATUS_EXE="
set "CHAT_EXE="
set "CLIENT_BIN=%ROOT%\build\bin\Release"
set "OPS_ROOT=%ROOT%\Memo_ops"
set "OPS_RUNTIME=%OPS_ROOT%\runtime"
set "ARTIFACT_ROOT=%OPS_ROOT%\artifacts"
set "RUN_ROOT=%OPS_RUNTIME%\services"
set "PID_ROOT=%ARTIFACT_ROOT%\runtime\pids"
set "CONSOLE_CP=936"
set "STATUS_CONFIG=%ROOT%\server\StatusServer\config.ini"

echo [INFO] ROOT=%ROOT%
echo [INFO] BUILD_BIN=%BUILD_BIN%
echo [INFO] WITH_CLIENT=%WITH_CLIENT%
echo [INFO] Parameters: %*

call :print_running_process_stamp GateServer.exe

if "%WITH_CLIENT%"=="1" (
  echo [INFO] Client check passed
) else (
  echo [INFO] Client skipped
)

goto :start_ops

:start_ops
echo [INFO] Ops section
goto :done

:print_running_process_stamp
set "RUN_EXE=%~1"
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0print_process_stamp.ps1" -ExeName "%RUN_EXE%"`) do echo %%I
if errorlevel 1 (
    echo [WARN] Unable to inspect running process image for %RUN_EXE%
)
exit /b 0

:done
echo [DONE] Script completed
exit /b 0
