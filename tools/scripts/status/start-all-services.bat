@echo off
REM ============================================================
REM  MemoChat 一键启动 (start-all-services.bat)
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0..\..\.."
set "PROJECT_ROOT=%CD%"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "RUNTIME_DIR=%PROJECT_ROOT%\infra\Memo_ops\runtime\services"
set "MEMO_OPS_ROOT=%PROJECT_ROOT%\infra\Memo_ops"
set "VARIFY_DIR=%PROJECT_ROOT%\apps\server\core\VarifyServer"

REM ---- Kafka / RabbitMQ 环境变量 ----
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1

echo ============================================================
echo   MemoChat 一键启动 (start-all-services.bat)
echo ============================================================
echo.

REM ---- MinIO ----
echo [STEP] Start MinIO object storage
call "%MEMO_OPS_ROOT%\bin\start_minio.bat"
echo.

REM ---- VarifyServer (Node.js) ----
echo [STEP] Start VarifyServer (Node.js)
if not exist "%VARIFY_DIR%\server.js" goto varify_missing
if exist "%VARIFY_DIR%\node_modules" goto varify_start
echo   [*] Installing Node.js dependencies...
cd /d "%VARIFY_DIR%" && npm install
:varify_start
call :launch_varify "VarifyServer-1" "50051" "8083" "VarifyServer1"
call :launch_varify "VarifyServer-2" "50383" "8087" "VarifyServer2"
goto varify_done
:varify_missing
echo   [X] VarifyServer not found
:varify_done
echo.

REM ---- 检查 / 自动部署 C++ 服务 ----
if not exist "%RUNTIME_DIR%\GateServer1\GateServer.exe" goto deploy_missing
if not exist "%RUNTIME_DIR%\AIServer\AIServer.exe" goto deploy_missing
goto deploy_done
:deploy_missing
echo   [WARN] C++ runtime missing, deploying services...
call "%~dp0deploy_services.bat"
if !ERRORLEVEL! neq 0 goto deploy_failed
echo.
:deploy_done
echo.
goto deploy_continue

:deploy_failed
echo   [X] deploy_services.bat failed; aborting startup
exit /b !ERRORLEVEL!

:deploy_continue

REM ---- C++ 服务 ----
echo [STEP] Start C++ backend services
call :launch_svc "%RUNTIME_DIR%\StatusServer1"    "StatusServer.exe" "StatusServer-1"   "config.ini"  "50052"
call :launch_svc "%RUNTIME_DIR%\StatusServer2"    "StatusServer.exe" "StatusServer-2"   "config.ini"  "50382"
call :launch_svc "%RUNTIME_DIR%\chatserver1"      "ChatServer.exe"   "ChatServer-1"     "config.ini"  "8090"
call :launch_svc "%RUNTIME_DIR%\chatserver2"      "ChatServer.exe"   "ChatServer-2"     "config.ini"  "8091"
call :launch_svc "%RUNTIME_DIR%\chatserver3"      "ChatServer.exe"   "ChatServer-3"     "config.ini"  "8092"
call :launch_svc "%RUNTIME_DIR%\chatserver4"      "ChatServer.exe"   "ChatServer-4"     "config.ini"  "8093"
call :launch_svc "%RUNTIME_DIR%\chatserver5"      "ChatServer.exe"   "ChatServer-5"     "config.ini"  "8094"
call :launch_svc "%RUNTIME_DIR%\chatserver6"      "ChatServer.exe"   "ChatServer-6"     "config.ini"  "8097"
call :launch_svc "%RUNTIME_DIR%\AIServer"         "AIServer.exe"     "AIServer"         "config.ini"  "8095"
call :launch_svc "%RUNTIME_DIR%\GateServer1"      "GateServer.exe"   "GateServer-1"     "config.ini"  "8080"
call :launch_svc "%RUNTIME_DIR%\GateServer2"      "GateServer.exe"   "GateServer-2"     "config.ini"  "8084"
echo.
echo 启动完成
exit /b 0

:launch_varify
setlocal
set "VARIFY_NAME=%~1"
set "VARIFY_GRPC_PORT=%~2"
set "VARIFY_HEALTH_PORT=%~3"
set "VARIFY_SERVICE=%~4"
powershell -NoProfile -Command "if (Get-NetTCPConnection -LocalPort %VARIFY_GRPC_PORT% -State Listen -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"
if !ERRORLEVEL! equ 0 (
    echo   [WARN] %VARIFY_NAME%: already running on port %VARIFY_GRPC_PORT%
    endlocal
    exit /b 0
)
set "VARIFY_LOG_DIR=%PROJECT_ROOT%\infra\Memo_ops\artifacts\logs\services\%VARIFY_SERVICE%"
echo   [*] Starting %VARIFY_NAME%...
start "%VARIFY_NAME%" /D "%VARIFY_DIR%" cmd /c "set MEMOCHAT_ENABLE_KAFKA=1&& set MEMOCHAT_ENABLE_RABBITMQ=1&& set MEMOCHAT_VARIFYSERVER_PORT=%VARIFY_GRPC_PORT%&& set MEMOCHAT_HEALTH_PORT=%VARIFY_HEALTH_PORT%&& set MEMOCHAT_TELEMETRY_SERVICENAME=%VARIFY_SERVICE%&& set MEMOCHAT_LOG_DIR=%VARIFY_LOG_DIR%&& node server.js"
echo   [OK] %VARIFY_NAME% started
endlocal
exit /b 0

:launch_svc
setlocal
set "SVC_DIR=%~1"
set "SVC_EXE=%~2"
set "SVC_NAME=%~3"
set "SVC_CONFIG=%~4"
set "SVC_PORT=%~5"
set "SVC_FULL=%SVC_DIR%\%SVC_EXE%"
set "LOG_DIR=%PROJECT_ROOT%\infra\Memo_ops\artifacts\logs\services"
set "LOG_NAME=%SVC_NAME%"
set "LOG_NAME=%LOG_NAME::=_%"
set "LOG_NAME=%LOG_NAME: =_%"

if not exist "%SVC_FULL%" (
    echo   [X] %SVC_NAME%: not found %SVC_FULL%
    endlocal
    exit /b 0
)

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%" 2>nul

if not "%SVC_PORT%"=="" (
    powershell -NoProfile -Command "if (Get-NetTCPConnection -LocalPort %SVC_PORT% -State Listen -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"
    if !ERRORLEVEL! equ 0 (
        echo   [WARN] %SVC_NAME%: already running on port %SVC_PORT%
        endlocal
        exit /b 0
    )
)

echo   [*] Starting %SVC_NAME%...
start "%SVC_NAME%" /D "%SVC_DIR%" powershell -NoProfile -ExecutionPolicy Bypass -NoExit -File "%~dp0run-service-console.ps1" -ServiceDir "%SVC_DIR%" -ExeName "%SVC_EXE%" -ServiceName "%SVC_NAME%" -ConfigName "%SVC_CONFIG%" -LogOut "%LOG_DIR%\%LOG_NAME%_out.log" -LogErr "%LOG_DIR%\%LOG_NAME%_err.log"
echo   [OK] %SVC_NAME% started
endlocal
exit /b 0
