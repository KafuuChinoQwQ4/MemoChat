@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Usage:
REM   start_test_services.bat
REM   start_test_services.bat --no-client

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "SERVER_BIN_PRIMARY=%ROOT%\build-vcpkg-server\bin\Release"
set "SERVER_BIN_FALLBACK=%ROOT%\build\bin\Release"
set "BUILD_BIN=%SERVER_BIN_PRIMARY%"
set "CLIENT_BIN=%ROOT%\build\bin\Release"
set "RUN_ROOT=%ROOT%\build\run"
set "WITH_CLIENT=1"
set "CONSOLE_CP=936"

if /I "%~1"=="--no-client" set "WITH_CLIENT=0"

call :select_server_bin
if errorlevel 1 exit /b 1

echo [INFO] Root: %ROOT%
echo [INFO] Server bin: %BUILD_BIN%
echo [INFO] Client bin: %CLIENT_BIN%
echo [INFO] Console code page: %CONSOLE_CP%
echo.

if not exist "%BUILD_BIN%\GateServer.exe" (
  echo [ERROR] Missing %BUILD_BIN%\GateServer.exe
  echo [HINT] Build first with one of:
  echo [HINT]   cmake --preset msvc2022-vcpkg-server ^&^& cmake --build --preset msvc2022-vcpkg-server-release
  echo [HINT]   cmake --preset msvc2022-all ^&^& cmake --build --preset msvc2022-release
  exit /b 1
)

if not exist "%BUILD_BIN%\StatusServer.exe" (
  echo [ERROR] Missing %BUILD_BIN%\StatusServer.exe
  exit /b 1
)

if not exist "%BUILD_BIN%\ChatServer.exe" (
  echo [ERROR] Missing %BUILD_BIN%\ChatServer.exe
  exit /b 1
)

if not exist "%BUILD_BIN%\ChatServer2.exe" (
  echo [ERROR] Missing %BUILD_BIN%\ChatServer2.exe
  exit /b 1
)

where node >nul 2>nul
if errorlevel 1 (
  echo [ERROR] node not found in PATH.
  exit /b 1
)

where npm >nul 2>nul
if errorlevel 1 (
  echo [ERROR] npm not found in PATH.
  exit /b 1
)

if not exist "%ROOT%\server\VarifyServer\package.json" (
  echo [ERROR] Missing %ROOT%\server\VarifyServer\package.json
  exit /b 1
)

if not exist "%ROOT%\server\GateServer\config.ini" (
  echo [ERROR] Missing server config files.
  exit /b 1
)

call :stop_existing_services
if errorlevel 1 exit /b 1

call :ensure_varify_deps
if errorlevel 1 exit /b 1

if not exist "%RUN_ROOT%" mkdir "%RUN_ROOT%"

call :prepare_service StatusServer "%ROOT%\server\StatusServer\config.ini" "%BUILD_BIN%\StatusServer.exe"
if errorlevel 1 exit /b 1

call :prepare_service ChatServer "%ROOT%\server\ChatServer\config.ini" "%BUILD_BIN%\ChatServer.exe"
if errorlevel 1 exit /b 1

call :prepare_service ChatServer2 "%ROOT%\server\ChatServer2\config.ini" "%BUILD_BIN%\ChatServer2.exe"
if errorlevel 1 exit /b 1

call :prepare_service GateServer "%ROOT%\server\GateServer\config.ini" "%BUILD_BIN%\GateServer.exe"
if errorlevel 1 exit /b 1

echo [INFO] Starting VarifyServer (Node)...
start "VarifyServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%ROOT%\server\VarifyServer"" && node server.js"
call :wait_for_port VarifyServer 50051 20 "%ROOT%\server\VarifyServer\logs"
if errorlevel 1 exit /b 1

echo [INFO] Starting StatusServer...
start "StatusServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\StatusServer"" && StatusServer.exe"
call :wait_for_port StatusServer 50052 20 "%RUN_ROOT%\StatusServer\logs"
if errorlevel 1 exit /b 1
call :print_running_process_stamp StatusServer.exe
if errorlevel 1 exit /b 1

echo [INFO] Starting ChatServer...
start "ChatServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\ChatServer"" && ChatServer.exe"
call :wait_for_port ChatServer 8090 20 "%RUN_ROOT%\ChatServer\logs"
if errorlevel 1 exit /b 1
call :wait_for_port ChatServer-RPC 50055 20 "%RUN_ROOT%\ChatServer\logs"
if errorlevel 1 exit /b 1
call :print_running_process_stamp ChatServer.exe
if errorlevel 1 exit /b 1

echo [INFO] Starting ChatServer2...
start "ChatServer2" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\ChatServer2"" && ChatServer2.exe"
call :wait_for_port ChatServer2 8091 20 "%RUN_ROOT%\ChatServer2\logs"
if errorlevel 1 exit /b 1
call :wait_for_port ChatServer2-RPC 50056 20 "%RUN_ROOT%\ChatServer2\logs"
if errorlevel 1 exit /b 1
call :print_running_process_stamp ChatServer2.exe
if errorlevel 1 exit /b 1

echo [INFO] Starting GateServer...
start "GateServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\GateServer"" && GateServer.exe"
call :wait_for_port GateServer 8080 20 "%RUN_ROOT%\GateServer\logs"
if errorlevel 1 exit /b 1
call :print_running_process_stamp GateServer.exe
if errorlevel 1 exit /b 1

if "%WITH_CLIENT%"=="1" (
  if not exist "%CLIENT_BIN%\MemoChatQml.exe" (
    if exist "%BUILD_BIN%\MemoChatQml.exe" set "CLIENT_BIN=%BUILD_BIN%"
  )
  if exist "%CLIENT_BIN%\MemoChatQml.exe" (
    echo [INFO] Starting MemoChat QML client...
    start "MemoChatQml" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%CLIENT_BIN%"" && MemoChatQml.exe"
  ) else (
    echo [WARN] MemoChatQml.exe not found, skip client start.
    echo [HINT] Build client first: cmake --preset msvc2022-all ^&^& cmake --build --preset msvc2022-release --target MemoChatQml
  )
)

echo.
echo [DONE] Services started.
echo [INFO] GateServer HTTP: http://127.0.0.1:8080
echo [INFO] Close each opened window to stop each service.
exit /b 0

:wait_for_port
set "WAIT_NAME=%~1"
set "WAIT_PORT=%~2"
set "WAIT_SECS=%~3"
set "WAIT_LOGDIR=%~4"
set "WAIT_FOUND="
if "%WAIT_SECS%"=="" set "WAIT_SECS=15"
echo [INFO] Waiting for %WAIT_NAME% on port %WAIT_PORT%...
for /L %%I in (1,1,%WAIT_SECS%) do (
  set "WAIT_FOUND="
  for /f "usebackq delims=" %%L in (`netstat -ano -p TCP ^| findstr /R /C:":%WAIT_PORT% .*LISTENING"`) do (
    set "WAIT_FOUND=1"
  )
  if defined WAIT_FOUND goto :wait_for_port_ok
  timeout /t 1 >nul
)
echo [ERROR] %WAIT_NAME% failed to listen on port %WAIT_PORT% within %WAIT_SECS% seconds.
if not "%WAIT_LOGDIR%"=="" (
  echo [HINT] Check logs under %WAIT_LOGDIR%
)
exit /b 1

:wait_for_port_ok
echo [INFO] %WAIT_NAME% is listening on port %WAIT_PORT%.
exit /b 0

:stop_existing_services
echo [INFO] Stopping existing MemoChat service processes...
for %%P in (GateServer.exe StatusServer.exe ChatServer.exe ChatServer2.exe) do (
  taskkill /F /IM %%P >nul 2>nul
)
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ports=@(8080,50051,50052,8090,8091,50055,50056); Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue ^| Where-Object { $ports -contains $_.LocalPort } ^| Select-Object -ExpandProperty OwningProcess -Unique ^| ForEach-Object { Stop-Process -Id $_ -Force -ErrorAction SilentlyContinue }" >nul 2>nul
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_Process -Filter \"Name='node.exe'\" ^| Where-Object { $_.CommandLine -like '*VarifyServer*server.js*' } ^| ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }" >nul 2>nul
exit /b 0

:ensure_varify_deps
set "VARIFY_DIR=%ROOT%\server\VarifyServer"

if exist "%VARIFY_DIR%\node_modules\@grpc\grpc-js" (
  echo [INFO] VarifyServer dependencies found.
  exit /b 0
)

echo [INFO] Installing VarifyServer dependencies...
pushd "%VARIFY_DIR%" >nul 2>nul
if errorlevel 1 (
  echo [ERROR] Failed to enter %VARIFY_DIR%
  exit /b 1
)

if exist "package-lock.json" (
  call npm ci
  if errorlevel 1 (
    echo [WARN] npm ci failed, fallback to npm install...
    call npm install
    if errorlevel 1 (
      popd
      echo [ERROR] npm install failed for VarifyServer.
      exit /b 1
    )
  )
) else (
  call npm install
  if errorlevel 1 (
    popd
    echo [ERROR] npm install failed for VarifyServer.
    exit /b 1
  )
)

popd
if not exist "%VARIFY_DIR%\node_modules\@grpc\grpc-js" (
  echo [ERROR] Dependency check failed: @grpc/grpc-js is still missing.
  exit /b 1
)
echo [INFO] VarifyServer dependencies ready.
exit /b 0

:prepare_service
set "SVC_NAME=%~1"
set "SVC_CFG=%~2"
set "SVC_EXE=%~3"
set "SVC_DIR=%RUN_ROOT%\%SVC_NAME%"

if not exist "%SVC_CFG%" (
  echo [ERROR] Missing config: %SVC_CFG%
  exit /b 1
)

if not exist "%SVC_EXE%" (
  echo [ERROR] Missing exe: %SVC_EXE%
  exit /b 1
)

if not exist "%SVC_DIR%" mkdir "%SVC_DIR%"

call :print_file_stamp "Source %SVC_NAME%" "%SVC_EXE%"
copy /Y "%SVC_EXE%" "%SVC_DIR%\%~nx3" >nul
copy /Y "%SVC_CFG%" "%SVC_DIR%\config.ini" >nul
call :print_file_stamp "Prepared %SVC_NAME%" "%SVC_DIR%\%~nx3"

echo [INFO] Prepared %SVC_NAME% in %SVC_DIR%
exit /b 0

:select_server_bin
set "BUILD_BIN="
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$dirs = @('%SERVER_BIN_PRIMARY%', '%SERVER_BIN_FALLBACK%');" ^
  "$required = 'GateServer.exe','StatusServer.exe','ChatServer.exe','ChatServer2.exe';" ^
  "$complete = foreach ($dir in $dirs) {" ^
  "  if (-not (Test-Path $dir)) { continue }" ^
  "  $missing = $required | Where-Object { -not (Test-Path (Join-Path $dir $_)) };" ^
  "  if (-not $missing) { [pscustomobject]@{ Path = $dir; StatusServerTime = (Get-Item (Join-Path $dir 'StatusServer.exe')).LastWriteTimeUtc } }" ^
  "};" ^
  "if (-not $complete) { exit 1 }" ^
  "$best = $complete | Sort-Object StatusServerTime -Descending | Select-Object -First 1;" ^
  "Write-Host ('[INFO] Candidate server bins: ' + (($complete | ForEach-Object { '{0} [{1}]' -f $_.Path, $_.StatusServerTime.ToString('yyyy-MM-dd HH:mm:ss') }) -join '; '));" ^
  "$best = $best.Path;" ^
  "Write-Output $best"`) do (
  set "BUILD_BIN=%%I"
)
if not defined BUILD_BIN (
  echo [ERROR] No complete server binary directory found.
  echo [HINT] Checked:
  echo [HINT]   %SERVER_BIN_PRIMARY%
  echo [HINT]   %SERVER_BIN_FALLBACK%
  exit /b 1
)
call :print_file_stamp "Selected StatusServer" "%BUILD_BIN%\StatusServer.exe"
call :print_file_stamp "Selected ChatServer" "%BUILD_BIN%\ChatServer.exe"
call :print_file_stamp "Selected ChatServer2" "%BUILD_BIN%\ChatServer2.exe"
call :print_file_stamp "Selected GateServer" "%BUILD_BIN%\GateServer.exe"
exit /b 0

:print_running_process_stamp
set "RUN_EXE=%~1"
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$proc = Get-CimInstance Win32_Process -Filter \"Name='%RUN_EXE%'\" -ErrorAction SilentlyContinue | Select-Object -First 1;" ^
  "if (-not $proc) { exit 1 }" ^
  "$path = $proc.ExecutablePath;" ^
  "if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path $path)) { exit 1 }" ^
  "$item = Get-Item $path;" ^
  "Write-Output ('[INFO] Running {0}: {1} ({2} bytes, {3})' -f '%RUN_EXE%', $item.FullName, $item.Length, $item.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss'))"`) do echo %%I
if errorlevel 1 (
  echo [WARN] Unable to inspect running process image for %RUN_EXE%
)
exit /b 0

:print_file_stamp
set "STAMP_LABEL=%~1"
set "STAMP_PATH=%~2"
if not exist "%STAMP_PATH%" (
  echo [WARN] %STAMP_LABEL% missing: %STAMP_PATH%
  exit /b 0
)
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$item = Get-Item '%STAMP_PATH%';" ^
  "Write-Output ('[INFO] {0}: {1} ({2} bytes, {3})' -f '%STAMP_LABEL%', $item.FullName, $item.Length, $item.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss'))"`) do echo %%I
exit /b 0

:has_all_servers
set "CHK_DIR=%~1"
if not exist "%CHK_DIR%\GateServer.exe" exit /b 1
if not exist "%CHK_DIR%\StatusServer.exe" exit /b 1
if not exist "%CHK_DIR%\ChatServer.exe" exit /b 1
if not exist "%CHK_DIR%\ChatServer2.exe" exit /b 1
exit /b 0
