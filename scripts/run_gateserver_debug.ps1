Set-Location "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer"
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = "..\..\..\artifacts\logs\services\GateServer\GateServer_$timestamp.log"
Write-Host "Starting GateServer, output to: $logFile"
& ".\GateServer.exe" --config config.ini 2>&1 | Tee-Object -FilePath $logFile
