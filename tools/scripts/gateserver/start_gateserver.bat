@echo off
REM ============================================================
REM Start GateServer with MinIO from Arch Linux native Docker
REM MinIO must be running as memochat-minio container.
REM Prerequisites: Arch Docker running with memochat-minio up.
REM ============================================================
setlocal enabledelayedexpansion

if defined MEMOCHAT_ROOT (
    set "PROJECT_ROOT=%MEMOCHAT_ROOT%"
) else (
    pushd "%~dp0..\..\.." >nul
    set "PROJECT_ROOT=%CD%"
    popd >nul
)
set "GATESERVER_DIR=%PROJECT_ROOT%\Memo_ops\runtime\services\GateServer"
set "GATESERVER_CONFIG=%PROJECT_ROOT%\server\GateServer\config.ini"

REM MinIO credentials (must match the running memochat-minio container)
if not defined MINIO_ROOT_USER if defined MEMOCHAT_MINIO_ROOT_USER set "MINIO_ROOT_USER=%MEMOCHAT_MINIO_ROOT_USER%"
if not defined MINIO_ROOT_USER if defined MEMOCHAT_MINIO_ACCESSKEY set "MINIO_ROOT_USER=%MEMOCHAT_MINIO_ACCESSKEY%"
if not defined MINIO_ROOT_USER if defined MEMOCHAT_MINIO_ACCESS_KEY set "MINIO_ROOT_USER=%MEMOCHAT_MINIO_ACCESS_KEY%"
if not defined MINIO_ROOT_PASSWORD if defined MEMOCHAT_MINIO_ROOT_PASSWORD set "MINIO_ROOT_PASSWORD=%MEMOCHAT_MINIO_ROOT_PASSWORD%"
if not defined MINIO_ROOT_PASSWORD if defined MEMOCHAT_MINIO_SECRETKEY set "MINIO_ROOT_PASSWORD=%MEMOCHAT_MINIO_SECRETKEY%"
if not defined MINIO_ROOT_PASSWORD if defined MEMOCHAT_MINIO_SECRET_KEY set "MINIO_ROOT_PASSWORD=%MEMOCHAT_MINIO_SECRET_KEY%"
if not defined MINIO_ROOT_USER (
    echo [ERROR] Missing MinIO user. Set MEMOCHAT_MINIO_ROOT_USER.
    exit /b 1
)
if not defined MINIO_ROOT_PASSWORD (
    echo [ERROR] Missing MinIO password. Set MEMOCHAT_MINIO_ROOT_PASSWORD.
    exit /b 1
)

REM GateServer env
set "MINIO_ACCESS_KEY=%MINIO_ROOT_USER%"
set "MINIO_SECRET_KEY=%MINIO_ROOT_PASSWORD%"

REM MinIO client
set "MC_BIN=%~dp0Memo_ops\bin\mc.exe"

REM Check Docker MinIO on port 9000
echo [INFO] Checking MinIO (Docker) on port 9000...
netstat -ano | findstr ":9000 " | findstr "LISTENING" >nul 2>&1
if !ERRORLEVEL!==0 (
    echo [OK] MinIO (Docker) is running.
    goto :minio_ok
)

echo [ERROR] MinIO not found on port 9000.
echo [HINT] Ensure Arch Docker is running with memochat-minio container up.
echo [HINT] Run: tools\scripts\docker\arch-docker.cmd compose -f infra\deploy\local\docker-compose.yml up -d memochat-minio
exit /b 1

:minio_ok
REM Ensure buckets exist via mc.exe
if exist "!MC_BIN!" (
    "!MC_BIN!" alias set local http://127.0.0.1:9000 %MINIO_ROOT_USER% %MINIO_ROOT_PASSWORD% >nul 2>&1
    for %%B in (memochat-avatar memochat-file memochat-image memochat-video) do (
        "!MC_BIN!" mb --ignore-existing local/%%B >nul 2>&1
        "!MC_BIN!" anonymous set download local/%%B >nul 2>&1
    )
    echo [INFO] Buckets checked.
) else (
    echo [WARN] mc.exe not found. Skipping bucket creation.
    echo [WARN] You may need to create buckets manually via MinIO Console at http://127.0.0.1:9001
)

echo [INFO] Starting GateServer...
cd /d "%GATESERVER_DIR%"
"%GATESERVER_DIR%\GateServer.exe" --config="%GATESERVER_CONFIG%"
echo GateServer exit code: %ERRORLEVEL%
endlocal
