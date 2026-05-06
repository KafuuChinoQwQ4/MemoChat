# Legacy wrapper. Prefer the k6 HTTP benchmark:
# tools\loadtest\k6\run-http-bench.ps1 -Scenario login -ConfigPath tools\loadtest\python-loadtest\config.benchmark.json

param(
    [int]$Total = 500,
    [int]$Concurrency = 50,
    [string]$GateUrl = "http://127.0.0.1:8080"
)

$script:SuccessCount = 0
$script:FailedCount = 0
$script:Latencies = @()
$script:Errors = @{}

function Send-LoginRequest {
    param($Email, $Password)
    $body = @{
        email = $Email
        passwd = $Password
        client_ver = "2.0.0"
    }
    
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        $resp = Invoke-WebRequest -Uri "$GateUrl/user_login" -Method POST -Body ($body | ConvertTo-Json) -ContentType "application/json" -TimeoutSec 5 -UseBasicParsing
        $sw.Stop()
        $latency = $sw.ElapsedMilliseconds
        
        $content = $resp.Content | ConvertFrom-Json
        if ($content.error -eq 0) {
            return @{ Success = $true; Latency = $latency }
        } else {
            return @{ Success = $false; Latency = $latency; Error = $content.error }
        }
    } catch {
        $sw.Stop()
        return @{ Success = $false; Latency = $sw.ElapsedMilliseconds; Error = $_.Exception.Message }
    }
}

function Get-VerifyCode {
    param($Email)
    $body = @{ email = $Email }
    
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        $resp = Invoke-WebRequest -Uri "$GateUrl/get_varifycode" -Method POST -Body ($body | ConvertTo-Json) -ContentType "application/json" -TimeoutSec 5 -UseBasicParsing
        $sw.Stop()
        $latency = $sw.ElapsedMilliseconds
        
        if ($resp.StatusCode -eq 200) {
            return @{ Success = $true; Latency = $latency }
        } else {
            return @{ Success = $false; Latency = $latency }
        }
    } catch {
        $sw.Stop()
        return @{ Success = $false; Latency = $sw.ElapsedMilliseconds; Error = $_.Exception.Message }
    }
}

function Get-Percentile {
    param($Values, $Percentile)
    $sorted = $Values | Sort-Object
    $idx = [math]::Ceiling($sorted.Count * $Percentile / 100) - 1
    if ($idx -lt 0) { $idx = 0 }
    if ($idx -ge $sorted.Count) { $idx = $sorted.Count - 1 }
    return $sorted[$idx]
}

function Run-Benchmark {
    param(
        $Name,
        $Total,
        $Concurrency,
        $TestScript
    )
    
    Write-Host ""
    Write-Host "[$Name] $Total 请求, $Concurrency 并发" -ForegroundColor Cyan
    Write-Host "-" * 50
    
    $script:SuccessCount = 0
    $script:FailedCount = 0
    $script:Latencies = @()
    $script:Errors = @{}
    
    $accounts = @(
        @{ Email = "loadtest_01@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_02@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_03@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_04@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_05@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_06@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_07@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_08@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_09@example.com"; Password = "ChangeMe123!" },
        @{ Email = "loadtest_10@example.com"; Password = "ChangeMe123!" }
    )
    
    $startTime = Get-Date
    $jobs = @()
    
    for ($i = 0; $i -lt $Total; $i++) {
        $account = $accounts[$i % $accounts.Count]
        
        $job = Start-Job -ScriptBlock {
            param($Email, $Password, $TestType, $GateUrl)
            
            if ($TestType -eq "login") {
                $body = @{
                    email = $Email
                    passwd = $Password
                    client_ver = "2.0.0"
                }
            } else {
                $body = @{ email = $Email }
            }
            
            $uri = if ($TestType -eq "login") { "$GateUrl/user_login" } else { "$GateUrl/get_varifycode" }
            
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            try {
                $resp = Invoke-WebRequest -Uri $uri -Method POST -Body ($body | ConvertTo-Json) -ContentType "application/json" -TimeoutSec 5 -UseBasicParsing
                $sw.Stop()
                $latency = $sw.ElapsedMilliseconds
                
                if ($resp.StatusCode -eq 200) {
                    $content = $resp.Content | ConvertFrom-Json
                    if ($TestType -eq "login" -and $content.error -ne 0) {
                        return @{ Success = $false; Latency = $latency; Error = $content.error }
                    }
                    return @{ Success = $true; Latency = $latency }
                }
                return @{ Success = $false; Latency = $latency; Error = $resp.StatusCode }
            } catch {
                $sw.Stop()
                return @{ Success = $false; Latency = $sw.ElapsedMilliseconds; Error = $_.Exception.Message }
            }
        } -ArgumentList $account.Email, $account.Password, $TestType, $GateUrl
        
        $jobs += $job
        
        if ($jobs.Count -ge $Concurrency) {
            $completed = $jobs | Wait-Job -Any
            $jobs = $jobs | Where-Object { $_.State -eq 'Running' }
        }
    }
    
    $jobs | Wait-Job | Out-Null
    
    foreach ($job in $jobs) {
        $result = Receive-Job -Job $job
        Remove-Job -Job $job
        
        if ($result.Success) {
            $script:SuccessCount++
            $script:Latencies += $result.Latency
        } else {
            $script:FailedCount++
            $errKey = $result.Error.ToString()
            if (-not $script:Errors.ContainsKey($errKey)) {
                $script:Errors[$errKey] = 0
            }
            $script:Errors[$errKey]++
        }
    }
    
    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds
    $throughput = [math]::Round($Total / $duration, 2)
    
    $successRate = if ($Total -gt 0) { $script:SuccessCount / $Total * 100 } else { 0 }
    
    $avgLatency = if ($script:Latencies.Count -gt 0) { ($script:Latencies | Measure-Object -Average).Average } else { 0 }
    $minLatency = if ($script:Latencies.Count -gt 0) { ($script:Latencies | Measure-Object -Minimum).Minimum } else { 0 }
    $maxLatency = if ($script:Latencies.Count -gt 0) { ($script:Latencies | Measure-Object -Maximum).Maximum } else { 0 }
    $p50 = if ($script:Latencies.Count -gt 0) { Get-Percentile -Values $script:Latencies -Percentile 50 } else { 0 }
    $p90 = if ($script:Latencies.Count -gt 0) { Get-Percentile -Values $script:Latencies -Percentile 90 } else { 0 }
    $p98 = if ($script:Latencies.Count -gt 0) { Get-Percentile -Values $script:Latencies -Percentile 98 } else { 0 }
    $p99 = if ($script:Latencies.Count -gt 0) { Get-Percentile -Values $script:Latencies -Percentile 99 } else { 0 }
    
    Write-Host "  成功率: $([math]::Round($successRate, 2))%" -ForegroundColor $(if ($successRate -gt 95) { "Green" } else { "Yellow" })
    Write-Host "  吞吐: $throughput req/s"
    Write-Host "  延迟 (ms):"
    Write-Host "    平均: $([math]::Round($avgLatency, 2))"
    Write-Host "    最小: $([math]::Round($minLatency, 2))"
    Write-Host "    最大: $([math]::Round($maxLatency, 2))"
    Write-Host "    P50:  $([math]::Round($p50, 2))"
    Write-Host "    P90:  $([math]::Round($p90, 2))"
    Write-Host "    P98:  $([math]::Round($p98, 2))"
    Write-Host "    P99:  $([math]::Round($p99, 2))"
    
    if ($script:Errors.Count -gt 0) {
        Write-Host "  错误分布:" -ForegroundColor Red
        foreach ($err in $script:Errors.GetEnumerator()) {
            Write-Host "    $($err.Key): $($err.Value)" -ForegroundColor Red
        }
    }
    
    return @{
        Name = $Name
        Total = $Total
        Success = $script:SuccessCount
        Failed = $script:FailedCount
        SuccessRate = $successRate
        Throughput = $throughput
        AvgLatency = $avgLatency
        MinLatency = $minLatency
        MaxLatency = $maxLatency
        P50 = $p50
        P90 = $p90
        P98 = $p98
        P99 = $p99
    }
}

# Main
Write-Host "=" -NoNewline -ForegroundColor Cyan
Write-Host ("=" * 59) -ForegroundColor Cyan
Write-Host "MemoChat HTTP 协议压测" -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor Cyan

# Test 1: Get Verify Code
$result1 = Run-Benchmark -Name "HTTP 获取验证码" -Total 200 -Concurrency 40 -TestScript ${function:Get-VerifyCode}

# Test 2: HTTP Login
$result2 = Run-Benchmark -Name "HTTP 登录" -Total 500 -Concurrency 100 -TestScript ${function:Send-LoginRequest}

# Test 3: HTTP Login High Concurrency
$result3 = Run-Benchmark -Name "HTTP 登录 (高并发)" -Total 1000 -Concurrency 200 -TestScript ${function:Send-LoginRequest}

# Summary
Write-Host ""
Write-Host ("=" * 60) -ForegroundColor Cyan
Write-Host "压测结果汇总" -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor Cyan
Write-Host ""

$results = @($result1, $result2, $result3)
foreach ($r in $results) {
    Write-Host "$($r.Name):" -ForegroundColor Yellow
    Write-Host "  总请求: $($r.Total), 成功: $($r.Success), 失败: $($r.Failed)"
    Write-Host "  成功率: $([math]::Round($r.SuccessRate, 2))%"
    Write-Host "  吞吐: $($r.Throughput) req/s"
    Write-Host "  延迟 (ms): avg=$([math]::Round($r.AvgLatency, 2)), p50=$([math]::Round($r.P50, 2)), p90=$([math]::Round($r.P90, 2)), p99=$([math]::Round($r.P99, 2))"
    Write-Host ""
}

# Save to JSON
$reportDir = "D:\MemoChat-Qml\Memo_ops\artifacts\reports\loadtest"
if (-not (Test-Path $reportDir)) {
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
}

$reportPath = Join-Path $reportDir "http_benchmark_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
$results | ConvertTo-Json -Depth 10 | Out-File -FilePath $reportPath -Encoding UTF8
Write-Host "报告已保存到: $reportPath" -ForegroundColor Green
