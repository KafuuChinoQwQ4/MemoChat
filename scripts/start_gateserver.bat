@echo off
REM ============================================================
REM Start GateServer with MinIO from Docker Desktop
REM MinIO must be running as memochat-minio container.
REM Prerequisites: Docker Desktop running with memochat-minio up.
REM ============================================================
setlocal enabledelayedexpansion

REM MinIO credentials (must match docker-compose.yml memochat-minio)
set "MINIO_ROOT_USER=memochat_admin"
set "MINIO_ROOT_PASSWORD=MinioPass2026!"

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
echo [HINT] Ensure Docker Desktop is running with memochat-minio container up.
echo [HINT] Run: docker compose -f deploy\local\docker-compose.yml up -d memochat-minio
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
cd /d "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer"
"D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe" --config="D:\MemoChat-Qml-Drogon\server\GateServer\config.ini"
echo GateServer exit code: %ERRORLEVEL%
endlocal
