@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Usage:
REM   start_test_services.bat
REM   start_test_services.bat --no-client

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"
set "BACKGROUND=1"
set "WITH_OPS=1"
for %%A in (%*) do (
  if /I "%%~A"=="--foreground" set "BACKGROUND=0"
  if /I "%%~A"=="--no-client" set "WITH_CLIENT=0"
  if /I "%%~A"=="--skip-ops" set "WITH_OPS=0"
)

set "SERVER_BIN_PRIMARY=%ROOT%\build-vcpkg-server\bin\Release"
set "SERVER_BIN_FALLBACK=%ROOT%\build\bin\Release"
set "BUILD_BIN=%SERVER_BIN_PRIMARY%"
set "GATE_EXE="
set "STATUS_EXE="
set "CHAT_EXE="
set "CLIENT_BIN=%ROOT%\build\bin\Release"
set "OPS_ROOT=%ROOT%\Memo_ops"
set "OPS_RUNTIME=%OPS_ROOT%\runtime"
set "ARTIFACT_ROOT=%OPS_ROOT%\artifacts"
set "RUN_ROOT=%OPS_RUNTIME%\services"
set "PID_ROOT=%ARTIFACT_ROOT%\runtime\pids"
set "WITH_CLIENT=1"
set "CONSOLE_CP=936"
set "STATUS_CONFIG=%ROOT%\server\StatusServer\config.ini"

for %%A in (%*) do (
  if /I "%%~A"=="--no-client" set "WITH_CLIENT=0"
)

call :select_server_bin
if errorlevel 1 exit /b 1

echo [INFO] Using skill: memochat-multiservice-ops
echo [INFO] Root: %ROOT%
echo [INFO] Gate exe: %GATE_EXE%
echo [INFO] Status exe: %STATUS_EXE%
echo [INFO] Chat exe: %CHAT_EXE%
echo [INFO] Client bin: %CLIENT_BIN%
echo [INFO] Console code page: %CONSOLE_CP%
echo.

if not exist "%GATE_EXE%" (
  echo [ERROR] Missing %GATE_EXE%
  echo [HINT] Build first with one of:
  echo [HINT]   cmake --preset msvc2022-vcpkg-server ^&^& cmake --build --preset msvc2022-vcpkg-server-release
  echo [HINT]   cmake --preset msvc2022-all ^&^& cmake --build --preset msvc2022-release
  exit /b 1
)

if not exist "%STATUS_EXE%" (
  echo [ERROR] Missing %STATUS_EXE%
  exit /b 1
)

if not exist "%CHAT_EXE%" (
  echo [ERROR] Missing %CHAT_EXE%
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
  echo [ERROR] Missing %ROOT%\server\GateServer\config.ini
  exit /b 1
)

if not exist "%STATUS_CONFIG%" (
  echo [ERROR] Missing %STATUS_CONFIG%
  exit /b 1
)

call :load_cluster_nodes "%STATUS_CONFIG%"
if errorlevel 1 exit /b 1

echo [INFO] Enabled chat nodes: !CHAT_NODE_NAMES!

call "%ROOT%\scripts\windows\stop_test_services.bat"
if errorlevel 1 exit /b 1
call :assert_cluster_ports_available
if errorlevel 1 exit /b 1

call :ensure_varify_deps
if errorlevel 1 exit /b 1
call :verify_varify_config
if errorlevel 1 exit /b 1
call :warn_if_port_closed RabbitMQ 5672 "async verify delivery, status presence worker, gate cache invalidate worker, chat task bus will run degraded"
call :warn_if_port_closed Kafka 19092 "chat async event bus and audit side effects may fail or retry noisily"
call :warn_if_port_closed Zipkin 9411 "trace export will be unavailable"

if not exist "%RUN_ROOT%" mkdir "%RUN_ROOT%"
if not exist "%ARTIFACT_ROOT%\logs\services\GateServer" mkdir "%ARTIFACT_ROOT%\logs\services\GateServer"
if not exist "%ARTIFACT_ROOT%\logs\services\StatusServer" mkdir "%ARTIFACT_ROOT%\logs\services\StatusServer"
if not exist "%ARTIFACT_ROOT%\logs\services\chatserver1" mkdir "%ARTIFACT_ROOT%\logs\services\chatserver1"
if not exist "%ARTIFACT_ROOT%\logs\services\chatserver2" mkdir "%ARTIFACT_ROOT%\logs\services\chatserver2"
if not exist "%ARTIFACT_ROOT%\logs\services\chatserver3" mkdir "%ARTIFACT_ROOT%\logs\services\chatserver3"
if not exist "%ARTIFACT_ROOT%\logs\services\chatserver4" mkdir "%ARTIFACT_ROOT%\logs\services\chatserver4"
if not exist "%ARTIFACT_ROOT%\logs\services\VarifyServer" mkdir "%ARTIFACT_ROOT%\logs\services\VarifyServer"
if not exist "%ARTIFACT_ROOT%\logs\manual-start" mkdir "%ARTIFACT_ROOT%\logs\manual-start"
if not exist "%ARTIFACT_ROOT%\loadtest\runtime\accounts" mkdir "%ARTIFACT_ROOT%\loadtest\runtime\accounts"
if not exist "%ARTIFACT_ROOT%\loadtest\runtime\logs" mkdir "%ARTIFACT_ROOT%\loadtest\runtime\logs"
if not exist "%ARTIFACT_ROOT%\loadtest\runtime\reports" mkdir "%ARTIFACT_ROOT%\loadtest\runtime\reports"
if not exist "%PID_ROOT%" mkdir "%PID_ROOT%"

call :prepare_service StatusServer "%ROOT%\server\StatusServer\config.ini" "%STATUS_EXE%"
if errorlevel 1 exit /b 1

for /L %%I in (1,1,!CHAT_NODE_COUNT!) do (
  set "NODE_NAME=!CHAT_NODE_%%I_NAME!"
  set "NODE_CFG=%ROOT%\server\ChatServer\!NODE_NAME!.ini"
  if not exist "!NODE_CFG!" (
    echo [ERROR] Missing chat node config: !NODE_CFG!
    exit /b 1
  )
  call :prepare_service !NODE_NAME! "!NODE_CFG!" "%CHAT_EXE%"
  if errorlevel 1 exit /b 1
)

call :prepare_service GateServer "%ROOT%\server\GateServer\config.ini" "%GATE_EXE%"
if errorlevel 1 exit /b 1

echo [INFO] Starting VarifyServer (Node)...
call :launch_process "VarifyServer" "node" "server.js" "%ROOT%\server\VarifyServer" "%ARTIFACT_ROOT%\logs\manual-start\VarifyServer.out.log" "%ARTIFACT_ROOT%\logs\manual-start\VarifyServer.err.log" "%PID_ROOT%\VarifyServer.pid"
if errorlevel 1 exit /b 1
call :wait_for_port VarifyServer 50051 20 "%ARTIFACT_ROOT%\logs\services\VarifyServer"
if errorlevel 1 exit /b 1

echo [INFO] Starting StatusServer...
call :launch_process "StatusServer" "%RUN_ROOT%\StatusServer\StatusServer.exe" "" "%RUN_ROOT%\StatusServer" "%ARTIFACT_ROOT%\logs\manual-start\StatusServer.out.log" "%ARTIFACT_ROOT%\logs\manual-start\StatusServer.err.log" "%PID_ROOT%\StatusServer.pid"
if errorlevel 1 exit /b 1
call :wait_for_port StatusServer 50052 20 "%ARTIFACT_ROOT%\logs\services\StatusServer"
if errorlevel 1 exit /b 1
call :print_running_process_stamp StatusServer.exe

for /L %%I in (1,1,!CHAT_NODE_COUNT!) do (
  set "NODE_NAME=!CHAT_NODE_%%I_NAME!"
  set "NODE_TCP_PORT=!CHAT_NODE_%%I_TCP_PORT!"
  set "NODE_RPC_PORT=!CHAT_NODE_%%I_RPC_PORT!"
  echo [INFO] Starting !NODE_NAME!...
  call :launch_process "!NODE_NAME!" "%RUN_ROOT%\!NODE_NAME!\ChatServer.exe" "--config .\config.ini" "%RUN_ROOT%\!NODE_NAME!" "%ARTIFACT_ROOT%\logs\manual-start\!NODE_NAME!.out.log" "%ARTIFACT_ROOT%\logs\manual-start\!NODE_NAME!.err.log" "%PID_ROOT%\!NODE_NAME!.pid"
  if errorlevel 1 exit /b 1
  call :wait_for_port !NODE_NAME! !NODE_TCP_PORT! 20 "%ARTIFACT_ROOT%\logs\services\!NODE_NAME!"
  if errorlevel 1 exit /b 1
  call :wait_for_port !NODE_NAME!-RPC !NODE_RPC_PORT! 20 "%ARTIFACT_ROOT%\logs\services\!NODE_NAME!"
  if errorlevel 1 exit /b 1
)

call :print_chat_process_count

echo [INFO] Starting GateServer...
call :launch_process "GateServer" "%RUN_ROOT%\GateServer\GateServer.exe" "" "%RUN_ROOT%\GateServer" "%ARTIFACT_ROOT%\logs\manual-start\GateServer.out.log" "%ARTIFACT_ROOT%\logs\manual-start\GateServer.err.log" "%PID_ROOT%\GateServer.pid"
if errorlevel 1 exit /b 1
call :wait_for_port GateServer 8080 20 "%ARTIFACT_ROOT%\logs\services\GateServer"
if errorlevel 1 exit /b 1
call :print_running_process_stamp GateServer.exe

if "%WITH_CLIENT%"=="1" (
  if not exist "%CLIENT_BIN%\MemoChatQml.exe" (
    if exist "%BUILD_BIN%\MemoChatQml.exe" set "CLIENT_BIN=%BUILD_BIN%"
  )
  if exist "%CLIENT_BIN%\MemoChatQml.exe" (
    echo [INFO] Starting MemoChat QML client...
    call :launch_process "MemoChatQml" "%CLIENT_BIN%\MemoChatQml.exe" "" "%CLIENT_BIN%" "%ARTIFACT_ROOT%\logs\manual-start\MemoChatQml.out.log" "%ARTIFACT_ROOT%\logs\manual-start\MemoChatQml.err.log" "%PID_ROOT%\MemoChatQml.pid"
  ) else (
    echo [WARN] MemoChatQml.exe not found, skip client start.
    echo [HINT] Build client first: cmake --preset msvc2022-all ^&^& cmake --build --preset msvc2022-release --target MemoChatQml
  )
)

echo [INFO] Starting Memo_ops platform...
if "%WITH_OPS%"=="1" (
  if "%WITH_CLIENT%"=="1" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%OPS_ROOT%\scripts\start_ops_platform.ps1" -Background -PidRoot "%PID_ROOT%"
  ) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%OPS_ROOT%\scripts\start_ops_platform.ps1" -NoClient -Background -PidRoot "%PID_ROOT%"
  )
  if errorlevel 1 exit /b 1
) else (
  echo [INFO] Memo_ops platform start skipped.
)

echo.
echo [DONE] Services started.
echo [INFO] GateServer HTTP: http://127.0.0.1:8080
echo [INFO] Chat cluster nodes: !CHAT_NODE_NAMES!
echo [INFO] Ops runtime: %OPS_RUNTIME%
echo [INFO] Artifacts root: %ARTIFACT_ROOT%
if "%BACKGROUND%"=="1" echo [INFO] Services were launched in background mode. Logs are under %ARTIFACT_ROOT%\logs\manual-start
exit /b 0

:launch_process
set "PROC_NAME=%~1"
set "PROC_FILE=%~2"
set "PROC_ARGS=%~3"
set "PROC_WORKDIR=%~4"
set "PROC_OUT=%~5"
set "PROC_ERR=%~6"
set "PROC_PID=%~7"
if not exist "%PROC_WORKDIR%" (
  echo [ERROR] Missing working directory for %PROC_NAME%: %PROC_WORKDIR%
  exit /b 1
)
if "%PROC_FILE%"=="node" goto :launch_process_run
if not exist "%PROC_FILE%" (
  echo [ERROR] Missing executable for %PROC_NAME%: %PROC_FILE%
  exit /b 1
)
:launch_process_run
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\scripts\windows\start_managed_process.ps1" -FilePath "%PROC_FILE%" -ArgumentList "%PROC_ARGS%" -WorkingDirectory "%PROC_WORKDIR%" -StdOutPath "%PROC_OUT%" -StdErrPath "%PROC_ERR%" -PidFile "%PROC_PID%" -Hidden >nul
if errorlevel 1 (
  echo [ERROR] Failed to launch %PROC_NAME%
  exit /b 1
)
set "PROC_LAUNCHED_PID="
if exist "%PROC_PID%" set /p PROC_LAUNCHED_PID=<"%PROC_PID%"
if not defined PROC_LAUNCHED_PID (
  echo [ERROR] Failed to launch %PROC_NAME%
  exit /b 1
)
echo [INFO] %PROC_NAME% PID: %PROC_LAUNCHED_PID%
set "PROC_LAUNCHED_PID="
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
  >nul ping -n 2 127.0.0.1
)
echo [ERROR] %WAIT_NAME% failed to listen on port %WAIT_PORT% within %WAIT_SECS% seconds.
if not "%WAIT_LOGDIR%"=="" (
  echo [HINT] Check logs under %WAIT_LOGDIR%
)
exit /b 1

:wait_for_port_ok
echo [INFO] %WAIT_NAME% is listening on port %WAIT_PORT%.
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

:verify_varify_config
set "VARIFY_DIR=%ROOT%\server\VarifyServer"
pushd "%VARIFY_DIR%" >nul 2>nul
if errorlevel 1 (
  echo [ERROR] Failed to enter %VARIFY_DIR%
  exit /b 1
)
node -e "require('./config.js')"
if errorlevel 1 (
  popd
  echo [ERROR] VarifyServer config bootstrap failed.
  echo [HINT] Fix server\VarifyServer\config.js or config.json before starting services.
  exit /b 1
)
popd
echo [INFO] VarifyServer config bootstrap passed.
exit /b 0

:warn_if_port_closed
set "WARN_NAME=%~1"
set "WARN_PORT=%~2"
set "WARN_HINT=%~3"
set "WARN_FOUND="
for /f "usebackq delims=" %%L in (`netstat -ano -p TCP ^| findstr /R /C:":%WARN_PORT% .*LISTENING"`) do (
  set "WARN_FOUND=1"
)
if not defined WARN_FOUND (
  echo [WARN] %WARN_NAME% is not listening on port %WARN_PORT%.
  if not "%WARN_HINT%"=="" echo [WARN] %WARN_HINT%
)
exit /b 0

:assert_cluster_ports_available
call :assert_port_available VarifyServer 50051
if errorlevel 1 exit /b 1
call :assert_port_available StatusServer 50052
if errorlevel 1 exit /b 1
call :assert_port_available GateServer 8080
if errorlevel 1 exit /b 1
for /L %%I in (1,1,!CHAT_NODE_COUNT!) do (
  call :assert_port_available !CHAT_NODE_%%I_NAME! !CHAT_NODE_%%I_TCP_PORT!
  if errorlevel 1 exit /b 1
  call :assert_port_available !CHAT_NODE_%%I_NAME!-RPC !CHAT_NODE_%%I_RPC_PORT!
  if errorlevel 1 exit /b 1
)
exit /b 0

:assert_port_available
set "PORT_NAME=%~1"
set "PORT_VALUE=%~2"
set "PORT_PID="
for /f "tokens=5" %%P in ('netstat -ano -p TCP ^| findstr /R /C:":%PORT_VALUE% .*LISTENING"') do (
  set "PORT_PID=%%P"
)
if defined PORT_PID (
  echo [ERROR] %PORT_NAME% port %PORT_VALUE% is already in use by PID %PORT_PID%.
  echo [HINT] Stop the stale process before retrying start_test_services.bat.
  exit /b 1
)
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

:load_cluster_nodes
set "CLUSTER_CONFIG=%~1"
for /L %%I in (1,1,32) do (
  set "CHAT_NODE_%%I_NAME="
  set "CHAT_NODE_%%I_TCP_PORT="
  set "CHAT_NODE_%%I_RPC_PORT="
)
set "CHAT_NODE_COUNT="
set "CHAT_NODE_NAMES="
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ini='%CLUSTER_CONFIG%';" ^
  "if (-not (Test-Path $ini)) { throw 'missing config file' }" ^
  "$sections=@{}; $section='';" ^
  "foreach ($raw in Get-Content $ini) {" ^
  "  $line=$raw.Trim();" ^
  "  if (-not $line -or $line.StartsWith(';') -or $line.StartsWith('#')) { continue }" ^
  "  if ($line -match '^\[(.+)\]$') { $section=$Matches[1].Trim(); if (-not $sections.ContainsKey($section)) { $sections[$section]=@{} }; continue }" ^
  "  $parts=$line.Split('=',2);" ^
  "  if ($section -and $parts.Count -eq 2) { $sections[$section][$parts[0].Trim()]=$parts[1].Trim() }" ^
  "}" ^
  "$cluster=$sections['Cluster'];" ^
  "if (-not $cluster) { throw 'missing [Cluster] section' }" ^
  "$nodes=((($cluster['Nodes']) -split ',') | ForEach-Object { $_.Trim() } | Where-Object { $_ });" ^
  "if (-not $nodes) { throw 'empty Cluster/Nodes' }" ^
  "$enabled=@();" ^
  "foreach ($nodeId in $nodes) {" ^
  "  $node=$sections[$nodeId];" ^
  "  if (-not $node) { throw ('missing node section [' + $nodeId + ']') }" ^
  "  $enabledValue=if ($node.ContainsKey('Enabled')) { [string]$node['Enabled'] } else { 'true' };" ^
  "  if ($enabledValue -match '^(?i:false|0|no|off)$') { continue }" ^
  "  $name=[string]$node['Name']; $tcpPort=[string]$node['TcpPort']; $rpcPort=[string]$node['RpcPort'];" ^
  "  if ([string]::IsNullOrWhiteSpace($name) -or [string]::IsNullOrWhiteSpace($tcpPort) -or [string]::IsNullOrWhiteSpace($rpcPort)) { throw ('incomplete node section [' + $nodeId + ']') }" ^
  "  $enabled += [pscustomobject]@{ Name=$name; TcpPort=$tcpPort; RpcPort=$rpcPort };" ^
  "}" ^
  "if (-not $enabled) { throw 'no enabled chat nodes found' }" ^
  "Write-Output ('set CHAT_NODE_COUNT=' + $enabled.Count);" ^
  "Write-Output ('set CHAT_NODE_NAMES=' + (($enabled | ForEach-Object { $_.Name }) -join ','));" ^
  "$index=1;" ^
  "foreach ($node in $enabled) {" ^
  "  Write-Output ('set CHAT_NODE_' + $index + '_NAME=' + $node.Name);" ^
  "  Write-Output ('set CHAT_NODE_' + $index + '_TCP_PORT=' + $node.TcpPort);" ^
  "  Write-Output ('set CHAT_NODE_' + $index + '_RPC_PORT=' + $node.RpcPort);" ^
  "  $index++;" ^
  "}"`) do %%I
if not defined CHAT_NODE_COUNT (
  echo [ERROR] Failed to load chat node list from %CLUSTER_CONFIG%
  exit /b 1
)
exit /b 0

:select_server_bin
set "BUILD_BIN="
set "GATE_EXE="
set "STATUS_EXE="
set "CHAT_EXE="
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\scripts\select_server_bins.ps1" -PrimaryDir "%SERVER_BIN_PRIMARY%" -FallbackDir "%SERVER_BIN_FALLBACK%"`) do (
  %%I
)
if not defined GATE_EXE (
  echo [ERROR] No GateServer.exe found.
  echo [HINT] Checked:
  echo [HINT]   %SERVER_BIN_PRIMARY%
  echo [HINT]   %SERVER_BIN_FALLBACK%
  exit /b 1
)
if not defined STATUS_EXE (
  echo [ERROR] No StatusServer.exe found.
  echo [HINT] Checked:
  echo [HINT]   %SERVER_BIN_PRIMARY%
  echo [HINT]   %SERVER_BIN_FALLBACK%
  exit /b 1
)
if not defined CHAT_EXE (
  echo [ERROR] No ChatServer.exe found.
  echo [HINT] Checked:
  echo [HINT]   %SERVER_BIN_PRIMARY%
  echo [HINT]   %SERVER_BIN_FALLBACK%
  exit /b 1
)
for %%I in ("%CHAT_EXE%") do set "BUILD_BIN=%%~dpI"
if "%BUILD_BIN:~-1%"=="\" set "BUILD_BIN=%BUILD_BIN:~0,-1%"
call :print_file_stamp "Selected GateServer" "%GATE_EXE%"
call :print_file_stamp "Selected StatusServer" "%STATUS_EXE%"
call :print_file_stamp "Selected ChatServer" "%CHAT_EXE%"
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

:print_chat_process_count
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$count = (Get-Process -Name 'ChatServer' -ErrorAction SilentlyContinue | Measure-Object).Count;" ^
  "Write-Output ('[INFO] Running ChatServer.exe instances: ' + $count)"`) do echo %%I
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
