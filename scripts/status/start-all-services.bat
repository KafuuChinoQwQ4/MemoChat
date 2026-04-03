@echo off
REM ============================================================
REM  MemoChat 一键启动 (start-all-services.bat)
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0..\.."
set "PROJECT_ROOT=%CD%"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "RUNTIME_DIR=%PROJECT_ROOT%\Memo_ops\runtime\services"
set "MEMO_OPS_ROOT=%PROJECT_ROOT%\Memo_ops"
set "VARIFY_DIR=%PROJECT_ROOT%\server\VarifyServer"

REM ---- Kafka / RabbitMQ 环境变量 ----
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1

echo ============================================================
echo   MemoChat 一键启动 (start-all-services.bat)
echo ============================================================
echo.

REM ---- MinIO ----
echo [步骤] 启动 MinIO 对象存储
call "%MEMO_OPS_ROOT%\bin\start_minio.bat"
echo.

REM ---- VarifyServer (Node.js) ----
echo [步骤] 启动 VarifyServer (Node.js)
if exist "%VARIFY_DIR%\server.js" (
    if not exist "%VARIFY_DIR%\node_modules" (
        echo   [*] 安装 Node.js 依赖...
        cd /d "%VARIFY_DIR%" && npm install
    )
    set MEMOCHAT_HEALTH_PORT=8082
    start "VarifyServer" cmd /c "cd /d "%VARIFY_DIR%" && set MEMOCHAT_ENABLE_KAFKA=1 && set MEMOCHAT_ENABLE_RABBITMQ=1 && set MEMOCHAT_HEALTH_PORT=8082 && node server.js"
    echo   [OK] VarifyServer started
) else (
    echo   [X] VarifyServer not found
)
echo.

REM ---- 检查 / 自动部署 C++ 服务 ----
if not exist "%RUNTIME_DIR%\GateServer\GateServer.exe" (
    echo   [!] 未找到 GateServer.exe，正在自动部署...
    call "%~dp0deploy_services.bat"
    echo.
)
echo.

REM ---- C++ 服务 ----
echo [步骤] 启动 C++ 后端服务
call :launch_svc "%RUNTIME_DIR%\GateServer"       "GateServer.exe"   "GateServer"       "config.ini"  "8080"
call :launch_svc "%RUNTIME_DIR%\StatusServer"     "StatusServer.exe" "StatusServer"     "config.ini"  "50052"
call :launch_svc "%RUNTIME_DIR%\chatserver1"      "ChatServer.exe"   "ChatServer-1"     "config.ini"  "8090"
call :launch_svc "%RUNTIME_DIR%\chatserver2"      "ChatServer.exe"   "ChatServer-2"     "config.ini"  "8091"
call :launch_svc "%RUNTIME_DIR%\chatserver3"      "ChatServer.exe"   "ChatServer-3"     "config.ini"  "8092"
call :launch_svc "%RUNTIME_DIR%\chatserver4"      "ChatServer.exe"   "ChatServer-4"     "config.ini"  "8093"
echo.
echo 启动完成
goto :eof

:launch_svc
setlocal
set "SVC_DIR=%~1"
set "SVC_EXE=%~2"
set "SVC_NAME=%~3"
set "SVC_CONFIG=%~4"
set "SVC_PORT=%~5"
set "SVC_FULL=%SVC_DIR%\%SVC_EXE%"

if not exist "%SVC_FULL%" (
    echo   [X] %SVC_NAME%: not found %SVC_FULL%
    endlocal
    exit /b 0
)

if not "%SVC_PORT%"=="" (
    powershell -NoProfile -Command "Get-NetTCPConnection -LocalPort %SVC_PORT% -State Listen -ErrorAction SilentlyContinue | Out-Null; if ($LASTEXITCODE -eq 0) { exit 0 } else { exit 1 }"
    if !ERRORLEVEL! equ 0 (
        echo   [!] %SVC_NAME%: already running on port %SVC_PORT%
        endlocal
        exit /b 0
    )
)

echo   [*] Starting %SVC_NAME%...
start "%SVC_NAME%" cmd /c "cd /d "%SVC_DIR%" && set MEMOCHAT_ENABLE_KAFKA=1 && set MEMOCHAT_ENABLE_RABBITMQ=1 && "%SVC_EXE%" --config "%SVC_CONFIG%""
echo   [OK] %SVC_NAME% started
endlocal
exit /b 0
