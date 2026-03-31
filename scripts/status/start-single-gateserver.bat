@echo off
REM ============================================================
REM  MemoChat 单 GateServer 启动脚本
REM  只启动: StatusServer + ChatServer1 + 一个 GateServer
REM ============================================================
setlocal enabledelayedexpansion
set "PROJECT_ROOT=D:\MemoChat-Qml-Drogon"
set "RUNTIME_DIR=%PROJECT_ROOT%\Memo_ops\runtime\services"
set "MEMO_OPS_ROOT=%PROJECT_ROOT%\Memo_ops"
echo  ============================================================
echo   MemoChat - 单 GateServer 启动
echo  ============================================================
echo.

REM ---- MinIO ----
echo [步骤] 启动 MinIO 对象存储
call "%MEMO_OPS_ROOT%\bin\start_minio.bat"
echo.

REM ---- C++ 服务 (只启动一个 GateServer) ----
echo [步骤] 启动 C++ 后端服务

REM StatusServer
call :launch "StatusServer"      "%RUNTIME_DIR%\StatusServer"  "StatusServer.exe"     "StatusServer"      "50052"

REM ChatServer 1-4
call :launch "ChatServer-1"      "%RUNTIME_DIR%\chatserver1"    "ChatServer.exe"      "ChatServer-1"      "8090"
call :launch "ChatServer-2"      "%RUNTIME_DIR%\chatserver2"    "ChatServer.exe"      "ChatServer-2"      "8091"
call :launch "ChatServer-3"      "%RUNTIME_DIR%\chatserver3"    "ChatServer.exe"      "ChatServer-3"      "8092"
call :launch "ChatServer-4"      "%RUNTIME_DIR%\chatserver4"    "ChatServer.exe"      "ChatServer-4"      "8093"

REM 只启动一个 GateServer (GateServerHttp1.1 端口 8081)
call :launch "GateServerHttp1.1" "%RUNTIME_DIR%\GateServerHttp2" "GateServerHttp1.1.exe" "GateServerHttp1.1" "8081"

echo.
echo 启动完成
goto :eof

:launch
set "SVC_FULL=%~2\%~3"
if not exist "%SVC_FULL%" (
    echo   [X] %~1: not found %SVC_FULL%
    exit /b 0
)
powershell -NoProfile -Command "Get-NetTCPConnection -LocalPort %~4 -State Listen -ErrorAction SilentlyContinue | Out-Null; if ($?) { exit 0 } else { exit 1 }"
if !ERRORLEVEL! equ 0 (
    echo   [!] %~1: already running on port %~4
    exit /b 0
)
start "%~1" cmd /c "cd /d ^"%~2^" ^&^& ^"%~3^""
echo   [OK] %~1 started on port %~4
exit /b 0
