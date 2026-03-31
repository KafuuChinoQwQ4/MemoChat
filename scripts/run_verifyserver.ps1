Set-Location "D:\MemoChat-Qml-Drogon\server\VarifyServer"
if (-not (Test-Path "node_modules")) {
    Write-Host "Installing npm dependencies..."
    npm install
}
$env:MEMOCHAT_HEALTH_PORT = "8082"
Start-Process -FilePath "node" -ArgumentList "server.js" -WindowStyle Hidden -WorkingDirectory "D:\MemoChat-Qml-Drogon\server\VarifyServer"
Write-Host "VarifyServer started"
