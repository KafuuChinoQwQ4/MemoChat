param(
    [string]$Config = ".\config.json",
    [string]$AccountsCsv = "",
    [ValidateSet("auth", "friend", "group", "history", "media", "call", "mysql", "redis", "login", "all")]
    [string]$Scenario = "all"
)

$ErrorActionPreference = "Stop"
$Config = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Config)
if ($AccountsCsv) {
    $AccountsCsv = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($AccountsCsv)
}
Set-Location -Path $PSScriptRoot

$configDir = Split-Path -Parent $Config
$configJson = Get-Content $Config -Raw | ConvertFrom-Json
$reportRoot = if ($env:MEMOCHAT_LOADTEST_REPORT_DIR) {
    $env:MEMOCHAT_LOADTEST_REPORT_DIR
} elseif ($configJson.report_dir) {
    Join-Path $configDir ([string]$configJson.report_dir)
} else {
    Join-Path $PSScriptRoot "reports"
}

if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    throw "python not found in PATH"
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$suiteDir = Join-Path $reportRoot ("suite_" + $timestamp)
New-Item -ItemType Directory -Force -Path $suiteDir | Out-Null

$reports = New-Object System.Collections.Generic.List[object]

function Invoke-LoadScript {
    param(
        [string]$ScriptName,
        [string]$ReportName,
        [string[]]$ExtraArgs = @(),
        [switch]$UseAccountsCsv
    )

    $reportPath = Join-Path $suiteDir $ReportName
    $args = @($ScriptName, "--config", $Config, "--report-path", $reportPath)
    if ($UseAccountsCsv -and $AccountsCsv) {
        $args += @("--accounts-csv", $AccountsCsv)
    }
    if ($ExtraArgs.Count -gt 0) {
        $args += $ExtraArgs
    }

    Write-Host "[STEP] $ScriptName"
    python @args
    if ($LASTEXITCODE -ne 0) {
        throw "$ScriptName failed with exit code $LASTEXITCODE"
    }

    if (Test-Path $reportPath) {
        $reports.Add([pscustomobject]@{
            path = $reportPath
            data = Get-Content $reportPath -Raw | ConvertFrom-Json
        })
    }
}

function Run-AuthScenario {
    Invoke-LoadScript ".\http_verify_code_load.py" "01_http_verify_code.json"
    Invoke-LoadScript ".\http_register_load.py" "02_http_register.json" -UseAccountsCsv
    Invoke-LoadScript ".\http_reset_password_load.py" "03_http_reset_password.json" -UseAccountsCsv
}

function Run-LoginScenario {
    Invoke-LoadScript ".\http_login_load.py" "04_http_login.json" -UseAccountsCsv
    Invoke-LoadScript ".\tcp_login_load.py" "05_tcp_login.json" -UseAccountsCsv
}

function Run-FriendScenario {
    Invoke-LoadScript ".\tcp_friendship_load.py" "06_tcp_friendship.json" -UseAccountsCsv
}

function Run-GroupScenario {
    Invoke-LoadScript ".\tcp_group_ops_load.py" "07_tcp_group_ops.json" -UseAccountsCsv
}

function Run-HistoryScenario {
    Invoke-LoadScript ".\tcp_history_ack_load.py" "08_tcp_history_ack.json" -UseAccountsCsv
}

function Run-MediaScenario {
    Invoke-LoadScript ".\media_load.py" "09_media_load.json" -UseAccountsCsv
}

function Run-CallScenario {
    Invoke-LoadScript ".\tcp_call_invite_load.py" "10_tcp_call_invite.json" -UseAccountsCsv
}

function Run-MySqlScenario {
    Invoke-LoadScript ".\mysql_capacity_load.py" "11_mysql_capacity.json" -UseAccountsCsv
}

function Run-RedisScenario {
    Invoke-LoadScript ".\redis_capacity_load.py" "12_redis_capacity.json"
}

Write-Host "[INFO] MemoChat load suite start"
Write-Host "[INFO] Config: $Config"
Write-Host "[INFO] Scenario: $Scenario"
if ($AccountsCsv) {
    Write-Host "[INFO] Accounts CSV: $AccountsCsv"
}

switch ($Scenario) {
    "auth" {
        Run-AuthScenario
    }
    "friend" {
        Run-FriendScenario
    }
    "group" {
        Run-GroupScenario
    }
    "history" {
        Run-HistoryScenario
    }
    "media" {
        Run-MediaScenario
    }
    "call" {
        Run-CallScenario
    }
    "mysql" {
        Run-MySqlScenario
    }
    "redis" {
        Run-RedisScenario
    }
    "login" {
        Run-LoginScenario
    }
    "all" {
        Run-AuthScenario
        Run-LoginScenario
        Run-FriendScenario
        Run-GroupScenario
        Run-HistoryScenario
        Run-MediaScenario
        Run-CallScenario
        Run-MySqlScenario
        Run-RedisScenario
    }
}

$totalSuccess = 0
$totalFailed = 0
foreach ($entry in $reports) {
    $totalSuccess += [int]$entry.data.summary.success
    $totalFailed += [int]$entry.data.summary.failed
}

$suiteSummary = [ordered]@{
    scenario = $Scenario
    started_at = $timestamp
    config = (Resolve-Path $Config).Path
    reports = @($reports | ForEach-Object { $_.path })
    totals = [ordered]@{
        tests = $reports.Count
        success = $totalSuccess
        failed = $totalFailed
    }
    breakdown = @()
}

foreach ($entry in $reports) {
    $report = $entry.data
    $suiteSummary.breakdown += [ordered]@{
        scenario = $report.scenario
        test = $report.test
        success = $report.summary.success
        failed = $report.summary.failed
        success_rate = $report.summary.success_rate
        throughput_rps = $report.summary.throughput_rps
        p95_ms = $report.summary.latency_ms.p95
        report_path = $entry.path
        top_errors = $report.top_errors
    }
}

$suiteSummaryPath = Join-Path $suiteDir "suite_summary.json"
$suiteSummary | ConvertTo-Json -Depth 8 | Set-Content -Path $suiteSummaryPath -Encoding UTF8

Write-Host "[DONE] Suite summary: $suiteSummaryPath"
Write-Host "[DONE] Reports directory: $suiteDir"
