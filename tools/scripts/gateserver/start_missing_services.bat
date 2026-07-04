@echo off
if defined MEMOCHAT_ROOT (
    set "PROJECT_ROOT=%MEMOCHAT_ROOT%"
) else (
    pushd "%~dp0..\..\.." >nul
    set "PROJECT_ROOT=%CD%"
    popd >nul
)
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1
set "RUNTIME_DIR=%PROJECT_ROOT%\Memo_ops\runtime\services"

echo Starting ChatServer-1...
start "ChatServer-1" cmd /k "cd /d %RUNTIME_DIR%\chatserver1 && set MEMOCHAT_ENABLE_KAFKA=1 && set MEMOCHAT_ENABLE_RABBITMQ=1 && ChatServer.exe --config config.ini"

echo All services started.
