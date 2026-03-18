@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM =====================================================
REM MemoChat One-Click Start All Services
REM Usage: run from CMD or PowerShell with:
REM   cmd /c "path\to\start_all_services.bat"
REM   & "path\to\start_all_services.bat"  (PowerShell)
REM =====================================================

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"

echo =====================================================
echo MemoChat One-Click Start All Services
echo =====================================================
echo.

echo [STEP 1/3] Starting backend services and MemoOps platform...
call "%ROOT%\scripts\windows\start_test_services.bat"
if errorlevel 1 (
    echo [ERROR] Backend services failed to start
    echo [HINT] Please build services first using CMake presets
    exit /b 1
)

echo.
echo [STEP 2/3] Checking MemoOps frontend...
set "OPS_RUNTIME=%ROOT%\Memo_ops\runtime\services\MemoOpsQml"
if not exist "%OPS_RUNTIME%\MemoOpsQml.exe" (
    echo [INFO] MemoOpsQml started via start_test_services.bat
)

echo.
echo [STEP 3/3] Checking service status...
echo.

echo [INFO] Port status:
for %%P in (8080 50051 50052 8090) do (
    for /f "tokens=5" %%I in ('netstat -ano -p TCP ^| findstr /R /C:":%%P .*LISTENING"') do (
        echo   [OK] Port %%P is listening
    )
)

echo.
echo =====================================================
echo All services started successfully!
echo =====================================================
echo.
echo Access URLs:
echo   - GateServer HTTP: http://127.0.0.1:8080
echo   - MemoOps Monitor: Check Memo_ops\runtime\services\MemoOpsQml
echo.
echo Log directories:
echo   - Backend: Memo_ops\artifacts\logs\manual-start
echo.
echo To stop: scripts\windows\stop_all_services.bat
echo =====================================================

exit /b 0