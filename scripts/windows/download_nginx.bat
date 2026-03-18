@echo off
REM Nginx Download Script for MemoChat
REM Run this script to download nginx

setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"
set "NGINX_DIR=%ROOT%\nginx"

if not exist "%NGINX_DIR%" mkdir "%NGINX_DIR%"

echo Downloading Nginx for Windows...
echo.

REM Download nginx 1.26.2 stable (no QUIC)
set "NGINX_VERSION=1.26.2"
set "DOWNLOAD_URL=https://nginx.org/download/nginx-%NGINX_VERSION%.zip"

echo URL: %DOWNLOAD_URL%
echo.

powershell -Command "Invoke-WebRequest -Uri '%DOWNLOAD_URL%' -OutFile '%NGINX_DIR%\nginx.zip'"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Download failed!
    exit /b 1
)

echo Extracting...
powershell -Command "Expand-Archive -Path '%NGINX_DIR%\nginx.zip' -DestinationPath '%NGINX_DIR%' -Force"

REM Move files to nginx directory
if exist "%NGINX_DIR%\nginx-%NGINX_VERSION%" (
    move /Y "%NGINX_DIR%\nginx-%NGINX_VERSION%\*" "%NGINX_DIR%\" >nul
    rmdir /Q /S "%NGINX_DIR%\nginx-%NGINX_VERSION%" 2>nul
)

del /Q "%NGINX_DIR%\nginx.zip"

echo.
echo Nginx installed to: %NGINX_DIR%
echo.

REM Test nginx
if exist "%NGINX_DIR%\nginx.exe" (
    "%NGINX_DIR%\nginx.exe" -v
    echo.
    echo Ready! Run: scripts\windows\start_nginx.bat --test
) else (
    echo [ERROR] nginx.exe not found after extraction
    exit /b 1
)
