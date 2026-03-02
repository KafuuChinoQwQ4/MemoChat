@echo off
setlocal EnableExtensions

echo [INFO] Stopping MemoChat test services...

for %%P in (GateServer.exe StatusServer.exe ChatServer.exe ChatServer2.exe MemoChatQml.exe MemoChat.exe) do (
  taskkill /F /IM %%P >nul 2>nul
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "$ports=@(8080,50051,50052,8090,8091,50055,50056); Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $ports -contains $_.LocalPort } | Select-Object -ExpandProperty OwningProcess -Unique | ForEach-Object { Stop-Process -Id $_ -Force -ErrorAction SilentlyContinue }" >nul 2>nul
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_Process -Filter \"Name='node.exe'\" | Where-Object { $_.CommandLine -like '*VarifyServer*server.js*' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }" >nul 2>nul

echo [DONE] MemoChat services stopped.
exit /b 0
