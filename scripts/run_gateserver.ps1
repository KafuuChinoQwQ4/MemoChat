Set-Location "D:\MemoChat-Qml\Memo_ops\runtime\services\GateServer"
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"
& ".\GateServer.exe" --config config.ini
