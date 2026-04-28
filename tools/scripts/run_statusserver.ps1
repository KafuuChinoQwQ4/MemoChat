Set-Location "$PSScriptRoot\..\Memo_ops\runtime\services\StatusServer"
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"
Start-Process -FilePath ".\StatusServer.exe" -ArgumentList "--config config.ini" -WindowStyle Normal
Write-Host "StatusServer started"
