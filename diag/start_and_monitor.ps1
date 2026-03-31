# Monitor script - starts all services with log redirection
$ErrorActionPreference = "Stop"
$diagDir = "D:\MemoChat-Qml-Drogon\diag"

# === 1. 清理残留进程 ===
Write-Host "=== 清理残留进程 ==="
$svcExes = @("GateServer","StatusServer","ChatServer")
foreach ($exe in $svcExes) {
    Get-Process -Name $exe -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host "  Killing old $($_.Name) PID=$($_.Id)"
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
}
Write-Host ""

# === 2. 清空旧日志 ===
Get-ChildItem $diagDir -Filter "*.log" -ErrorAction SilentlyContinue | ForEach-Object {
    ""> $_.FullName
}
Write-Host "=== 启动所有服务 ==="

# === 3. 启动 VarifyServer ===
$env:MEMOCHAT_HEALTH_PORT = "8082"
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"
$varifyProc = Start-Process -FilePath "node" -ArgumentList "server.js" -WorkingDirectory "D:\MemoChat-Qml-Drogon\server\VarifyServer" -PassThru -RedirectStandardOutput "$diagDir\varify_out.log" -RedirectStandardError "$diagDir\varify_err.log" -WindowStyle Hidden
Write-Host "[VarifyServer] PID: $($varifyProc.Id)"
Start-Sleep -Seconds 3

# === 4. 启动 GateServer ===
$gateProc = Start-Process -FilePath "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe" -ArgumentList "--config","config.ini" -WorkingDirectory "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer" -PassThru -RedirectStandardOutput "$diagDir\gate_out.log" -RedirectStandardError "$diagDir\gate_err.log" -WindowStyle Hidden
Write-Host "[GateServer] PID: $($gateProc.Id)"
Start-Sleep -Seconds 2

# === 5. 启动 StatusServer ===
$statusProc = Start-Process -FilePath "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\StatusServer\StatusServer.exe" -ArgumentList "--config","config.ini" -WorkingDirectory "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\StatusServer" -PassThru -RedirectStandardOutput "$diagDir\status_out.log" -RedirectStandardError "$diagDir\status_err.log" -WindowStyle Hidden
Write-Host "[StatusServer] PID: $($statusProc.Id)"
Start-Sleep -Seconds 2

# === 6. 启动 ChatServers ===
$chatProcs = @()
$chatNodes = @("chatserver1","chatserver2","chatserver3","chatserver4")
foreach ($node in $chatNodes) {
    $proc = Start-Process -FilePath "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\$node\ChatServer.exe" -ArgumentList "--config","config.ini" -WorkingDirectory "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\$node" -PassThru -RedirectStandardOutput "$diagDir\chat_$node.log" -RedirectStandardError "$diagDir\chat_${node}_err.log" -WindowStyle Hidden
    $chatProcs += $proc
    Write-Host "[$node] PID: $($proc.Id)"
    Start-Sleep -Seconds 1
}

Write-Host ""
Write-Host "All services started. Waiting 5s for initialization..."
Start-Sleep -Seconds 5

# === 7. 验证端口 ===
Write-Host ""
Write-Host "=== Port Status ==="
$ports = @(8080,50051,50052,8090,8091,8092,8093,50055,50056,50057,50058)
foreach ($port in $ports) {
    $conn = Get-NetTCPConnection -LocalPort $port -ErrorAction SilentlyContinue | Where-Object { $_.State -eq "Listen" }
    if ($conn) {
        Write-Host "  :$port - LISTENING (PID $($conn[0].OwningProcess))"
    } else {
        Write-Host "  :$port - NOT LISTENING"
    }
}

# === 8. 持续监控进程存活 ===
Write-Host ""
Write-Host "=== 开始监控进程存活 ==="
$allProcs = @($varifyProc,$gateProc,$statusProc) + $chatProcs
$checkCount = 0

while ($true) {
    Start-Sleep -Seconds 5
    $checkCount++
    $now = Get-Date -Format "HH:mm:ss"

    # 检查每个进程
    $anyDead = $false
    foreach ($p in $allProcs) {
        if ($p -eq $null) { continue }
        $running = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
        if (-not $running) {
            Write-Host "[$now] DEAD! PID=$($p.Id) Name=$($p.ProcessName)"
            $anyDead = $true
        }
    }

    # 每30秒打印一次存活状态
    if ($checkCount % 6 -eq 0) {
        Write-Host "[$now] Status check OK ($checkCount)"
    }
}
