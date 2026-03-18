@echo off
REM MemoChat Nginx Load Balancer Startup Script
REM Usage:
REM   start_nginx.bat          - Start nginx in foreground (for testing)
REM   start_nginx.bat --daemon - Start nginx as daemon

setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"
set "NGINX_DIR=%ROOT%\nginx"
set "NGINX_CONF=%NGINX_DIR%\conf\nginx.conf"
set "NGINX_EXE="
set "NGINX_LOG=%NGINX_DIR%\logs"

if not exist "%NGINX_CONF%" (
    echo [ERROR] Nginx config not found: %NGINX_CONF%
    exit /b 1
)

REM Find nginx.exe in common locations
if exist "%NGINX_DIR%\nginx.exe" (
    set "NGINX_EXE=%NGINX_DIR%\nginx.exe"
) else (
    where nginx >nul 2>nul
    if %ERRORLEVEL%==0 (
        set "NGINX_EXE=nginx"
    ) else (
        echo [ERROR] nginx.exe not found. Please install Nginx or place nginx.exe in %NGINX_DIR%
        exit /b 1
    )
)

REM Create logs directory if not exists
if not exist "%NGINX_LOG%" mkdir "%NGINX_LOG%"

echo Starting Nginx Load Balancer...
echo   Config: %NGINX_CONF%
echo   Logs:   %NGINX_LOG%

REM Parse command line arguments
set "DAEMON_MODE=0"
for %%A in (%*) do (
    if /I "%%~A"=="--daemon" set "DAEMON_MODE=1"
    if /I "%%~A"=="--test" (
        echo Testing nginx configuration...
        "%NGINX_EXE%" -t -c "%NGINX_CONF%"
        exit /b %ERRORLEVEL%
    )
)

if %DAEMON_MODE%==1 (
    echo Starting nginx in daemon mode...
    start /B "" "%NGINX_EXE%" -c "%NGINX_CONF%"
) else (
    echo Starting nginx in foreground (Ctrl+C to stop)...
    "%NGINX_EXE%" -c "%NGINX_CONF%"
)

echo Nginx started.
