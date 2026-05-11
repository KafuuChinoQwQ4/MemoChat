@echo off
REM ============================================================
REM 启动 MinIO 对象存储服务
REM
REM 方式: 通过 Arch Linux native Docker 容器方式运行 memochat-minio
REM
REM MinIO 凭证 (与 docker-compose.yml 保持一致):
REM   MINIO_ROOT_USER=memochat_admin
REM   MINIO_ROOT_PASSWORD=MinioPass2026!
REM
REM API:      http://127.0.0.1:9000
REM Console:  http://127.0.0.1:9001
REM ============================================================
setlocal enabledelayedexpansion

cd /d "%~dp0..\..\.."
set "PROJECT_ROOT=%CD%"
set "COMPOSE_FILE=%PROJECT_ROOT%\infra\deploy\local\docker-compose.yml"
set "CONTAINER_NAME=memochat-minio"
set "DOCKER=%PROJECT_ROOT%\tools\scripts\docker\arch-docker.cmd"

echo [INFO] 启动 MinIO 对象存储...
echo.

REM ---- 检查 Arch Docker 是否运行 ----
"%DOCKER%" info >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo [ERROR] Arch Docker 不可用，请在 archlinux 中启动 docker.service 后重试。
    exit /b 1
)

REM ---- 检查容器是否已在运行 ----
"%DOCKER%" ps --filter "name=%CONTAINER_NAME%" --format "{{.Names}}" | findstr /x "%CONTAINER_NAME%" >nul 2>&1
if !ERRORLEVEL! neq 0 goto :try_start
echo [OK] !CONTAINER_NAME! 已在运行
goto :check_health

REM ---- 检查容器是否存在 (已停止) ----
:try_start
"%DOCKER%" ps -a --filter "name=%CONTAINER_NAME%" --format "{{.Names}}" | findstr /x "%CONTAINER_NAME%" >nul 2>&1
if !ERRORLEVEL! equ 0 (
    echo [INFO] 容器 !CONTAINER_NAME! 存在，正在启动...
    "%DOCKER%" start %CONTAINER_NAME%
) else (
    echo [INFO] 容器 !CONTAINER_NAME! 不存在，正在创建...
    "%DOCKER%" compose -f "!COMPOSE_FILE!" up -d %CONTAINER_NAME%
)

if !ERRORLEVEL! neq 0 (
    echo [ERROR] MinIO 启动失败
    exit /b 1
)

:check_health
REM ---- 健康检查 ----
echo [INFO] 等待 MinIO 就绪...
set "MAX_WAIT=30"
set "COUNT=0"

:wait_loop
    set /a COUNT+=1
    if !COUNT! gtr !MAX_WAIT! (
        echo [WARN] MinIO 健康检查超时，但容器可能仍在启动
        goto :done
    )

    curl -s -f http://127.0.0.1:9000/minio/health/live >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo [OK] MinIO 健康检查通过
        goto :done
    )

    REM 每 2 秒检查一次
    timeout /t 2 /nobreak >nul 2>&1
    goto :wait_loop

:done
echo.
echo [INFO] MinIO API:      http://127.0.0.1:9000
echo [INFO] MinIO Console: http://127.0.0.1:9001
echo [INFO] Console 用户: memochat_admin
echo [INFO] Console 密码: MinioPass2026!
echo.
echo 启动完成
endlocal
exit /b 0
