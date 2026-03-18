@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM =====================================================
REM MemoChat One-Click Stop All Services
REM Includes: Backend + MemoOps Platform + Frontend
REM =====================================================

chcp 65001 >nul 2>nul
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"

echo =====================================================
echo MemoChat One-Click Stop All Services
echo =====================================================
echo.

REM Call original stop script
echo [STEP 1/2] Stopping backend services, MemoOps platform and frontend...
call "%ROOT%\scripts\windows\stop_test_services.bat"

echo.
echo [STEP 2/2] Verifying port release...
echo [INFO] Port check:

REM Check if ports are released
set "ALL_CLEARED=1"
for %%P in (8080 50051 50052 8090 8091 8092 8093) do (
    for /f "tokens=5" %%I in ('netstat -ano -p TCP ^| findstr /R /C:":%%P .*LISTENING" 2^>nul') do (
        echo   [WARN] Port %%P still in use
        set "ALL_CLEARED=0"
    )
)

if "%ALL_CLEARED%"=="1" (
    echo   [OK] All ports cleared
)

echo.
echo =====================================================
echo All services stopped!
echo =====================================================
echo.
echo To restart, run: scripts\windows\start_all_services.bat
echo =====================================================

exit /b 0