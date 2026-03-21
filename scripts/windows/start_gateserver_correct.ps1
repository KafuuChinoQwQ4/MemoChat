$ErrorActionPreference = "Stop"
$exePath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe"
$configPath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\config.ini"
$pidFile = "D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\runtime\pids\GateServer.pid"

$proc = Start-Process -FilePath $exePath -WorkingDirectory (Split-Path $exePath) -NoNewWindow -PassThru
$newPid = $proc.Id
Write-Host "New PID: $newPid"
Start-Sleep -Seconds 3
$newPid | Out-File -FilePath $pidFile -Encoding ASCII
Write-Host "PID file updated."

# Verify port 8080 is listening
$listening = netstat -ano | findstr "LISTENING" | findstr ":8080"
if ($listening) {
    Write-Host "Port 8080 is listening. GateServer is running."
} else {
    Write-Host "WARNING: Port 8080 is NOT listening yet."
}
