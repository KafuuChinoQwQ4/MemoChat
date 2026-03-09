@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "STATUS_CONFIG=%ROOT%\server\StatusServer\config.ini"

echo [INFO] Stopping MemoChat test services...

for %%P in (GateServer.exe StatusServer.exe ChatServer.exe MemoChatQml.exe) do (
  taskkill /F /IM %%P >nul 2>nul
)

set "PORT_LIST=8080,50051,50052"
call :try_load_cluster_ports "%STATUS_CONFIG%"
if defined CHAT_PORT_LIST (
  set "PORT_LIST=%PORT_LIST%,%CHAT_PORT_LIST%"
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "$ports=@(%PORT_LIST%); Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $ports -contains $_.LocalPort } | Select-Object -ExpandProperty OwningProcess -Unique | ForEach-Object { Stop-Process -Id $_ -Force -ErrorAction SilentlyContinue }" >nul 2>nul
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_Process -Filter \"Name='node.exe'\" | Where-Object { $_.CommandLine -like '*VarifyServer*server.js*' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }" >nul 2>nul

echo [DONE] MemoChat services stopped.
exit /b 0

:try_load_cluster_ports
set "CLUSTER_CONFIG=%~1"
set "CHAT_PORT_LIST="
if not exist "%CLUSTER_CONFIG%" exit /b 0
for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ini='%CLUSTER_CONFIG%';" ^
  "$sections=@{}; $section='';" ^
  "foreach ($raw in Get-Content $ini) {" ^
  "  $line=$raw.Trim();" ^
  "  if (-not $line -or $line.StartsWith(';') -or $line.StartsWith('#')) { continue }" ^
  "  if ($line -match '^\[(.+)\]$') { $section=$Matches[1].Trim(); if (-not $sections.ContainsKey($section)) { $sections[$section]=@{} }; continue }" ^
  "  $parts=$line.Split('=',2);" ^
  "  if ($section -and $parts.Count -eq 2) { $sections[$section][$parts[0].Trim()]=$parts[1].Trim() }" ^
  "}" ^
  "$cluster=$sections['Cluster'];" ^
  "if (-not $cluster) { exit 0 }" ^
  "$nodes=((($cluster['Nodes']) -split ',') | ForEach-Object { $_.Trim() } | Where-Object { $_ });" ^
  "$ports=@();" ^
  "foreach ($nodeId in $nodes) {" ^
  "  $node=$sections[$nodeId];" ^
  "  if (-not $node) { continue }" ^
  "  $enabledValue=if ($node.ContainsKey('Enabled')) { [string]$node['Enabled'] } else { 'true' };" ^
  "  if ($enabledValue -match '^(?i:false|0|no|off)$') { continue }" ^
  "  if ($node['TcpPort']) { $ports += [string]$node['TcpPort'] }" ^
  "  if ($node['RpcPort']) { $ports += [string]$node['RpcPort'] }" ^
  "}" ^
  "if ($ports) { Write-Output ('set CHAT_PORT_LIST=' + (($ports | Select-Object -Unique) -join ',')) }"`) do %%I
exit /b 0
