@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo Testing updated functions...
echo.

echo 1. Testing :print_running_process_stamp
call :print_running_process_stamp GateServer.exe
echo.

echo 2. Testing :print_chat_process_count
call :print_chat_process_count
echo.

echo 3. Testing :print_file_stamp
call :print_file_stamp "Test File" "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe"
echo.

echo All tests completed
exit /b 0

:print_running_process_stamp
set "RUN_EXE=%~1"
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0print_process_stamp.ps1" -ExeName "%RUN_EXE%"`) do echo %%I
if errorlevel 1 (
    echo [WARN] Unable to inspect running process image for %RUN_EXE%
)
exit /b 0

:print_chat_process_count
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0print_chat_count.ps1" -ExeName "ChatServer"`) do echo %%I
exit /b 0

:print_file_stamp
set "STAMP_LABEL=%~1"
set "STAMP_PATH=%~2"
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0print_file_stamp.ps1" -Label "%STAMP_LABEL%" -Path "%STAMP_PATH%"`) do echo %%I
if errorlevel 1 (
    echo [WARN] %STAMP_LABEL% missing: %STAMP_PATH%
)
exit /b 0
