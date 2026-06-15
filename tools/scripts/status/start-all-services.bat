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
set "DOCKER=%PROJECT_ROOT%\tools\scripts\docker\arch-docker.cmd"
set "LOCAL_COMPOSE_FILE=%PROJECT_ROOT%\infra\deploy\local\docker-compose.yml"

REM ---- Kafka / RabbitMQ 环境变量 ----
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1

echo ============================================================
echo   MemoChat 一键启动 (start-all-services.bat)
echo ============================================================
echo.

REM ---- Envoy Gateway ----
echo [STEP] Start Docker Envoy Gateway
if "%MEMOCHAT_START_ENVOY%"=="0" (
    echo   [SKIP] Envoy Gateway startup disabled
    goto envoy_done
)
if not exist "%LOCAL_COMPOSE_FILE%" (
    echo   [X] Local compose file not found: %LOCAL_COMPOSE_FILE%
    exit /b 1
)
call "%DOCKER%" compose -f "%LOCAL_COMPOSE_FILE%" up -d memochat-envoy-gateway
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
powershell -NoProfile -Command "$ok=$false; for($i=0;$i -lt 16;$i++){ try { $r=Invoke-WebRequest 'http://127.0.0.1/health' -UseBasicParsing -TimeoutSec 2; if($r.StatusCode -eq 200){ $ok=$true; break } } catch {}; Start-Sleep -Seconds 1 }; if($ok){ Write-Host '  [OK] Envoy Gateway ready at http://127.0.0.1/health' } else { Write-Host '  [WARN] Envoy Gateway did not answer /health yet' }"
:envoy_done
echo.

REM ---- MinIO ----
echo [STEP] Start MinIO object storage
call "%MEMO_OPS_ROOT%\bin\start_minio.bat"
echo.

REM ---- 检查 / 自动部署 C++ 服务 ----
if not exist "%RUNTIME_DIR%\VarifyServer1\VarifyServer.exe" goto deploy_missing
if not exist "%RUNTIME_DIR%\VarifyServer2\VarifyServer.exe" goto deploy_missing
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
call :launch_svc "%RUNTIME_DIR%\VarifyServer1"   "VarifyServer.exe" "VarifyServer-1"  "config.ini"  "50051"
call :launch_svc "%RUNTIME_DIR%\VarifyServer2"   "VarifyServer.exe" "VarifyServer-2"  "config.ini"  "48083"
call :launch_svc "%RUNTIME_DIR%\chatserver1"      "ChatServer.exe"   "ChatServer-1"     "config.ini"  "8090"
call :launch_svc "%RUNTIME_DIR%\chatserver2"      "ChatServer.exe"   "ChatServer-2"     "config.ini"  "8091"
call :launch_svc "%RUNTIME_DIR%\AIServer"         "AIServer.exe"     "AIServer"         "config.ini"  "8095"
call :launch_svc "%RUNTIME_DIR%\GateServer1"      "GateServer.exe"   "GateServer-1"     "config.ini"  "8080"
call :launch_svc "%RUNTIME_DIR%\GateServer2"      "GateServer.exe"   "GateServer-2"     "config.ini"  "8084"
echo   [INFO] Client HTTP traffic enters through Docker Envoy on 80 and 8443/tcp+udp
echo.
echo 启动完成
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
