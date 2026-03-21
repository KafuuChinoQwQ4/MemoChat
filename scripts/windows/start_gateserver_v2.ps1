$ErrorActionPreference = "Stop"
$exePath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe"
$configPath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\config.ini"
$pidFile = "D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\runtime\pids\GateServer.pid"

# Start the process with -PassThru
$proc = Start-Process -FilePath $exePath -ArgumentList "--config=""$configPath""" -PassThru

# Wait a bit for the process to start
Start-Sleep -Seconds 2

# Check if process is still running
if ($proc.HasExited) {
    Write-Host "ERROR: GateServer exited immediately with code $($proc.ExitCode)"
    exit 1
}

$newPid = $proc.Id
Write-Host "New PID: $newPid"

# Wait a bit more to see if it stays running
Start-Sleep -Seconds 2

if ($proc.HasExited) {
    Write-Host "ERROR: GateServer exited with code $($proc.ExitCode)"
    exit 1
}

# Write PID to file
$newPid | Out-File -FilePath $pidFile -Encoding ASCII
Write-Host "PID file updated."

# Wait more and check if process is still running
Start-Sleep -Seconds 3

if ($proc.HasExited) {
    Write-Host "ERROR: GateServer exited with code $($proc.ExitCode)"
    exit 1
}

# Verify port 8080 is listening
$listening = netstat -ano | findstr "LISTENING" | findstr ":8080"
if ($listening) {
    Write-Host "SUCCESS: Port 8080 is listening. GateServer is running."
} else {
    Write-Host "WARNING: Port 8080 is NOT listening yet. Process is running but may still be starting up."
}
