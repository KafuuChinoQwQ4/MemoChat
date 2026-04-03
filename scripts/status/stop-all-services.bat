@echo off
REM ============================================================
REM  MemoChat 一键停止所有可执行进程
REM  注意: 此脚本只停止 exe 进程，不关闭 Docker 容器
REM  Docker 容器需要手动停止或使用 docker compose down
REM
REM  停止顺序 (大致逆序):
REM    Tier 5: QML 客户端
REM    Tier 4: MemoOps 平台 (Python)
REM    Tier 3: VarifyServer (Node.js)
REM    Tier 2: C++ 后端服务
REM
REM  用法:
REM    stop-all-services.bat  仅停止所有 exe 进程 (保留 Docker 容器)
REM ============================================================
setlocal enabledelayedexpansion

cd /d "%~dp0..\.."
set "PROJECT_ROOT=%CD%"
set "MEMO_OPS_ROOT=%PROJECT_ROOT%\Memo_ops"

echo.
echo ============================================================
echo  MemoChat 一键停止 (仅停止 exe 进程)
echo ============================================================
echo.

REM ============================================================
REM Tier 5: QML 客户端
REM ============================================================
echo [步骤] 停止 QML 客户端
call :kill_by_name "MemoChatQml.exe"  "MemoChatQml"
call :kill_by_name "MemoOpsQml.exe"   "MemoOpsQml"

REM ============================================================
REM Tier 4: MemoOps 平台 (Python)
REM ============================================================
echo.
echo [步骤] 停止 MemoOps 平台 (Python)
set "OPS_STOP=%MEMO_OPS_ROOT%\scripts\stop_ops_platform.ps1"
if exist "!OPS_STOP!" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "!OPS_STOP!"
    echo   [OK] MemoOps 平台已停止
) else (
    REM 兜底: 手动杀 Python 进程
    echo   [警告] stop_ops_platform.ps1 未找到，尝试直接停止 Python 进程
    call :kill_by_name "python.exe" "MemoOpsServer/MemoOpsCollector"
)

REM ============================================================
REM Tier 3: VarifyServer (Node.js)
REM ============================================================
echo.
echo [步骤] 停止 VarifyServer (Node.js)
powershell -NoProfile -Command "$p = Get-NetTCPConnection -LocalPort 50051 -State Listen -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess; if ($p) { Stop-Process -Id $p -Force -ErrorAction SilentlyContinue; Write-Host '  [OK] VarifyServer (Node.js) 已停止' } else { Write-Host '  [-] VarifyServer 未在运行' }" 2>nul

REM ============================================================
REM Tier 2: C++ 后端服务
REM ============================================================
echo.
echo [步骤] 停止 C++ 后端服务

REM 按启动顺序逆序停止
call :kill_by_name "StatusServer.exe"       "StatusServer"
call :kill_by_name "GateServerHttp1.1.exe"  "GateServerHttp1.1"
call :kill_by_name "GateServer.exe"         "GateServer"
call :kill_by_name "ChatServer.exe"         "ChatServer (所有实例)"

REM ============================================================
REM 完成: 存活检查报告
REM ============================================================
echo.
echo ============================================================
echo  停止完成 — 存活检查
echo ============================================================
echo.
call :check "MemoChatQml.exe"       "MemoChatQml"
call :check "MemoOpsQml.exe"         "MemoOpsQml"
call :check "GateServerHttp1.1.exe" "GateServerHttp1.1"
call :check "GateServer.exe"          "GateServer"
call :check "ChatServer.exe"           "ChatServer"
call :check "StatusServer.exe"         "StatusServer"

echo.
echo  Docker 容器 (保持运行):
powershell -NoProfile -Command "docker ps --filter 'name=memochat' --format 'table {{.Names}}\t{{.Status}}' 2>&1; if ($LASTEXITCODE -ne 0) { Write-Host '  无 memochat 相关容器运行' }"

echo.
echo  重新启动 exe 进程:
echo   .\scripts\status\start-all-services.bat
echo.
echo  停止 Docker 容器 (如需):
echo   docker compose -f deploy\local\docker-compose.yml down
echo.
goto :eof

REM ============================================================
REM 辅助函数
REM ============================================================

REM 尝试按进程名停止
:kill_by_name
set "PROC_NAME=%~1"
set "DISPLAY_NAME=%~2"
tasklist /FI "IMAGENAME eq %PROC_NAME%" /FO CSV /NH 2>nul | findstr /i "%PROC_NAME%" >nul
if !ERRORLEVEL! equ 0 (
    taskkill /IM "%PROC_NAME%" /F >nul 2>&1
    echo   [OK] !DISPLAY_NAME! 已停止
) else (
    echo   [-] !DISPLAY_NAME! 未运行
)
exit /b 0

REM 检查并报告进程状态
:check
set "PROC_NAME=%~1"
set "DISPLAY_NAME=%~2"
tasklist /FI "IMAGENAME eq %PROC_NAME%" /FO CSV /NH 2>nul | findstr /i "%PROC_NAME%" >nul
if !ERRORLEVEL! equ 0 (
    echo   [!] %DISPLAY_NAME% 仍在运行
) else (
    echo   [OK] %DISPLAY_NAME% 已停止
)
exit /b 0
