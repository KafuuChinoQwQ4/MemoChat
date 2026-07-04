$ProjectRoot = if ($env:MEMOCHAT_ROOT) {
    $env:MEMOCHAT_ROOT
} else {
    (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
}
$ServiceDir = Join-Path $ProjectRoot "Memo_ops\runtime\services\GateServer"

Set-Location $ServiceDir
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"
& ".\GateServer.exe" --config config.ini
