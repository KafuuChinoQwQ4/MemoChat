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
set "GATE_EXE="
set "STATUS_EXE="
set "CHAT_EXE="
set "CLIENT_BIN=%ROOT%\build\bin\Release"
set "OPS_ROOT=%ROOT%\Memo_ops"
set "OPS_RUNTIME=%OPS_ROOT%\runtime"
set "RUN_ROOT=%OPS_RUNTIME%\services"
set "WITH_CLIENT=1"
set "CONSOLE_CP=936"
set "STATUS_CONFIG=%ROOT%\server\StatusServer\config.ini"

if /I "%~1"=="--no-client" set "WITH_CLIENT=0"

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

call "%ROOT%\stop_test_services.bat"
if errorlevel 1 exit /b 1

call :ensure_varify_deps
if errorlevel 1 exit /b 1

if not exist "%RUN_ROOT%" mkdir "%RUN_ROOT%"
if not exist "%OPS_RUNTIME%\varify\logs" mkdir "%OPS_RUNTIME%\varify\logs"
if not exist "%OPS_RUNTIME%\loadtest\logs" mkdir "%OPS_RUNTIME%\loadtest\logs"
if not exist "%OPS_RUNTIME%\loadtest\reports" mkdir "%OPS_RUNTIME%\loadtest\reports"

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
start "VarifyServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%ROOT%\server\VarifyServer"" && node server.js"
call :wait_for_port VarifyServer 50051 20 "%ROOT%\server\VarifyServer\logs"
if errorlevel 1 exit /b 1

echo [INFO] Starting StatusServer...
start "StatusServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\StatusServer"" && StatusServer.exe"
call :wait_for_port StatusServer 50052 20 "%RUN_ROOT%\StatusServer\logs"
if errorlevel 1 exit /b 1
call :print_running_process_stamp StatusServer.exe

for /L %%I in (1,1,!CHAT_NODE_COUNT!) do (
  set "NODE_NAME=!CHAT_NODE_%%I_NAME!"
  set "NODE_TCP_PORT=!CHAT_NODE_%%I_TCP_PORT!"
  set "NODE_RPC_PORT=!CHAT_NODE_%%I_RPC_PORT!"
  echo [INFO] Starting !NODE_NAME!...
  start "ChatServer-!NODE_NAME!" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\!NODE_NAME!"" && ChatServer.exe --config .\config.ini"
  call :wait_for_port !NODE_NAME! !NODE_TCP_PORT! 20 "%RUN_ROOT%\!NODE_NAME!\logs"
  if errorlevel 1 exit /b 1
  call :wait_for_port !NODE_NAME!-RPC !NODE_RPC_PORT! 20 "%RUN_ROOT%\!NODE_NAME!\logs"
  if errorlevel 1 exit /b 1
)

call :print_chat_process_count

echo [INFO] Starting GateServer...
start "GateServer" cmd /k "chcp %CONSOLE_CP%>nul && cd /d ""%RUN_ROOT%\GateServer"" && GateServer.exe"
call :wait_for_port GateServer 8080 20 "%RUN_ROOT%\GateServer\logs"
if errorlevel 1 exit /b 1
call :print_running_process_stamp GateServer.exe

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

echo [INFO] Starting Memo_ops platform...
if "%WITH_CLIENT%"=="1" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%OPS_ROOT%\scripts\start_ops_platform.ps1"
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%OPS_ROOT%\scripts\start_ops_platform.ps1" -NoClient
)
if errorlevel 1 exit /b 1

echo.
echo [DONE] Services started.
echo [INFO] GateServer HTTP: http://127.0.0.1:8080
echo [INFO] Chat cluster nodes: !CHAT_NODE_NAMES!
echo [INFO] Ops runtime: %OPS_RUNTIME%
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
