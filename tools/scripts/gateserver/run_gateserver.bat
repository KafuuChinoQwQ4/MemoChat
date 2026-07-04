@echo off
if defined MEMOCHAT_ROOT (
    set "PROJECT_ROOT=%MEMOCHAT_ROOT%"
) else (
    pushd "%~dp0..\..\.." >nul
    set "PROJECT_ROOT=%CD%"
    popd >nul
)
cd /d "%PROJECT_ROOT%\Memo_ops\runtime\services\GateServer"
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1
start "GateServer" GateServer.exe --config config.ini
