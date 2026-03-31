@echo off
REM ============================================================
REM 停止 MinIO 对象存储服务
REM
REM 方式: 停止 Docker Desktop 中的 memochat-minio 容器
REM ============================================================
setlocal enabledelayedexpansion

set "CONTAINER_NAME=memochat-minio"

echo [INFO] 停止 MinIO 对象存储...
echo.

REM ---- 检查 Docker 是否可用 ----
docker info >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo [警告] Docker 不可用，跳过 MinIO 停止
    goto :done
)

REM ---- 检查容器是否存在 ----
for /f "delims=" %%c in ('powershell -NoProfile -Command "docker ps --filter 'name=%CONTAINER_NAME%' --format '{{.Names}}' 2>&1"') do set "CONTAINER_FOUND=%%c"
if not defined CONTAINER_FOUND (
    echo [-] !CONTAINER_NAME! 未在运行
    goto :done
)

REM ---- 停止容器 (保留数据卷) ----
docker stop !CONTAINER_NAME! >nul 2>&1
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
