@echo off
REM ============================================================
REM 停止 MinIO 对象存储服务
REM
REM 方式: 停止 Arch Linux native Docker 中的 memochat-minio 容器
REM ============================================================
setlocal enabledelayedexpansion

set "CONTAINER_NAME=memochat-minio"
cd /d "%~dp0..\..\.."
set "PROJECT_ROOT=%CD%"
set "DOCKER=%PROJECT_ROOT%\tools\scripts\docker\arch-docker.cmd"

echo [INFO] 停止 MinIO 对象存储...
echo.

REM ---- 检查 Arch Docker 是否可用 ----
"%DOCKER%" info >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo [警告] Arch Docker 不可用，跳过 MinIO 停止
    goto :done
)

REM ---- 检查容器是否存在 ----
"%DOCKER%" ps --filter "name=%CONTAINER_NAME%" --format "{{.Names}}" | findstr /x "%CONTAINER_NAME%" >nul 2>&1
if !ERRORLEVEL! neq 0 goto :not_found
set "CONTAINER_FOUND=%CONTAINER_NAME%"
if not defined CONTAINER_FOUND (
:not_found
    echo [-] !CONTAINER_NAME! 未在运行
    goto :done
)

REM ---- 停止容器 (保留数据卷) ----
"%DOCKER%" stop !CONTAINER_NAME! >nul 2>&1
if !ERRORLEVEL! equ 0 (
    echo [OK] !CONTAINER_NAME! 已停止 (数据卷已保留)
) else (
    REM 容器存在但停止失败，不阻塞主流程
    echo [ERROR] 停止 !CONTAINER_NAME! 容器失败
)

:done
echo.
endlocal
exit /b 0
