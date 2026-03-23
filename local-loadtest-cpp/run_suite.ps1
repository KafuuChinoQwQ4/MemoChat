param(
    [string]$Config = ".\config.json",
    [string]$AccountsCsv = "",
    [ValidateSet("auth", "friend", "group", "history", "media", "call",
                 "postgresql", "mysql", "redis", "login",
                 "tcp", "quic", "http",
                 "all")]
    [string]$Scenario = "all",
    [int]$WarmupIterations = 10,
    [int]$PoolSize = 200
)

$ErrorActionPreference = "Stop"

# Resolve paths
$Config = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Config)
if ($AccountsCsv) {
    $AccountsCsv = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($AccountsCsv)
}
Set-Location -Path $PSScriptRoot

# Detect which engine to use for each scenario
# Python-only scenarios must use Python scripts (local-loadtest/*.py).
# Login scenarios (tcp/http/quic) can use the C++ binary via cpp_run.py.
$USE_CPP_BINARY = $true   # Set to $false to fall back to Python for login scenarios

function Get-PythonPath {
    $py = Get-Command python -ErrorAction SilentlyContinue
    if (-not $py) {
        throw "python not found in PATH — Python scenarios (auth/friend/group/history/media/call/postgresql/redis) require Python"
    }
    return $py.Source
}

function Get-CppBinary {
    $script_dir = $PSScriptRoot
    $candidates = @(
        (Join-Path $script_dir "build\Release\memochat_loadtest.exe"),
        (Join-Path $PSScriptRoot "build\Release\memochat_loadtest.exe")
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Get-ConfigReportDir($cfgPath) {
    $cfgDir = Split-Path -Parent $cfgPath
    try {
        $json = Get-Content $cfgPath -Raw | ConvertFrom-Json
        $rd = if ($env:MEMOCHAT_LOADTEST_REPORT_DIR) {
            $env:MEMOCHAT_LOADTEST_REPORT_DIR
        } elseif ($json.report_dir) {
            Join-Path $cfgDir ([string]$json.report_dir)
        } else {
            Join-Path $PSScriptRoot "reports"
        }
        # Resolve relative paths
        if (-not [System.IO.Path]::IsPathRooted($rd)) {
            $rd = Join-Path $cfgDir $rd
        }
        return $rd
    } catch {
        return Join-Path $PSScriptRoot "reports"
    }
}

$reportRoot = Get-ConfigReportDir $Config
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$suiteDir = Join-Path $reportRoot ("suite_" + $timestamp)
New-Item -ItemType Directory -Force -Path $suiteDir | Out-Null

# ---------------------------------------------------------------------------
# Invoke C++ binary directly for login scenarios
# ---------------------------------------------------------------------------
function Invoke-CppLogin {
    param(
        [string]$ScenarioName,  # "tcp", "http", or "quic"
        [string]$ReportName,
        [string[]]$ExtraArgs = @()
    )

    $reportPath = Join-Path $suiteDir $ReportName

    $cppExe = Get-CppBinary
    if (-not $cppExe) {
        Write-Host "[ERROR] C++ binary not found"
        throw "C++ binary not found"
    }

    $args = @(
        $cppExe,
        "--config", $Config,
        "--scenario", $ScenarioName,
        "--warmup", $WarmupIterations,
        "--pool-size", $PoolSize
    )
    if ($ExtraArgs.Count -gt 0) {
        $args += $ExtraArgs
    }

    Write-Host "[STEP] C++ $ScenarioName login (warmup=$WarmupIterations, pool=$PoolSize): $cppExe"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $args[0]
    $psi.Arguments = ($args[1..($args.Count-1)] -join ' ')
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.WorkingDirectory = (Split-Path -Parent $cppExe)

    $proc = [System.Diagnostics.Process]::Start($psi)
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    if ($stdout) { Write-Host $stdout }
    if ($stderr -and $proc.ExitCode -ne 0) { Write-Host $stderr }

    if ($proc.ExitCode -ne 0) {
        throw "C++ binary (scenario=$ScenarioName) failed with exit code $($proc.ExitCode)"
    }

    return $reportPath
}

# ---------------------------------------------------------------------------
# Invoke Python script for business scenarios
# ---------------------------------------------------------------------------
function Invoke-PythonScript {
    param(
        [string]$ScriptName,
        [string]$ReportName,
        [string[]]$ExtraArgs = @(),
        [switch]$UseAccountsCsv
    )

    $reportPath = Join-Path $suiteDir $ReportName
    $scriptPath = Join-Path $PSScriptRoot ".." "local-loadtest" $ScriptName

    if (-not (Test-Path $scriptPath)) {
        Write-Host "[SKIP] Script not found: $scriptPath"
        return $null
    }

    $args = @(
        (Get-PythonPath),
        $scriptPath,
        "--config", $Config,
        "--report-path", $reportPath
    )
    if ($UseAccountsCsv -and $AccountsCsv) {
        $args += @("--accounts-csv", $AccountsCsv)
    }
    if ($ExtraArgs.Count -gt 0) {
        $args += $ExtraArgs
    }

    Write-Host "[STEP] $ScriptName"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $args[0]
    $psi.Arguments = ($args[1..($args.Count-1)] -join ' ')
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.WorkingDirectory = (Split-Path -Parent $scriptPath)

    $proc = [System.Diagnostics.Process]::Start($psi)
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    if ($stdout) { Write-Host $stdout }
    if ($stderr -and $proc.ExitCode -ne 0) { Write-Host $stderr }

    if ($proc.ExitCode -ne 0) {
        throw "$ScriptName failed with exit code $($proc.ExitCode)"
    }

    return $reportPath
}

# ---------------------------------------------------------------------------
# Load report JSON (handles both C++ and Python report schemas)
# ---------------------------------------------------------------------------
function Get-ReportData {
    param([string]$Path)
    if (-not $Path -or -not (Test-Path $Path)) { return $null }
    try {
        return Get-Content $Path -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

# ---------------------------------------------------------------------------
# Pre-test warmup: ensure Redis cache is hot before running tests
# ---------------------------------------------------------------------------
function Invoke-PreTestWarmup {
    param(
        [string]$ScenarioName,
        [int]$Iterations
    )

    if ($Iterations -le 0) {
        Write-Host "[INFO] Pre-test warmup disabled (warmup=$Iterations)"
        return
    }

    Write-Host "[INFO] Pre-test warmup for $ScenarioName ($Iterations iterations)"

    $cppExe = Get-CppBinary
    if (-not $cppExe) {
        Write-Host "[WARN] C++ binary not found, skipping warmup"
        return
    }

    $args = @(
        $cppExe,
        "--config", $Config,
        "--scenario", $ScenarioName,
        "--warmup", $Iterations,
        "--pool-size", $PoolSize
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $args[0]
    $psi.Arguments = ($args[1..($args.Count-1)] -join ' ')
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.WorkingDirectory = (Split-Path -Parent $cppExe)

    $proc = [System.Diagnostics.Process]::Start($psi)
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    if ($stdout) { Write-Host $stdout }
    if ($stderr -and $proc.ExitCode -ne 0) { Write-Host $stderr }

    Write-Host "[INFO] Pre-test warmup completed"
}

# ---------------------------------------------------------------------------
# Scenario runners
# ---------------------------------------------------------------------------
function Run-AuthScenario {
    $reports = @{}
    $r = Invoke-PythonScript "http_verify_code_load.py" "01_http_verify_code.json"
    $reports["http_verify_code"] = Get-ReportData $r
    $r = Invoke-PythonScript "http_register_load.py" "02_http_register.json" -UseAccountsCsv
    $reports["http_register"] = Get-ReportData $r
    $r = Invoke-PythonScript "http_reset_password_load.py" "03_http_reset_password.json" -UseAccountsCsv
    $reports["http_reset_password"] = Get-ReportData $r
    return $reports
}

function Run-LoginScenario {
    # Pre-test warmup: populate Redis cache before measuring
    Invoke-PreTestWarmup -ScenarioName "tcp" -Iterations $WarmupIterations

    # TCP and HTTP via C++ binary
    $reports = @{}
    $extra = if ($AccountsCsv) { @("--config", $Config) } else { @() }
    $r = Invoke-CppLogin "tcp" "04_tcp_login.json" $extra
    $reports["tcp_login"] = Get-ReportData $r
    $r = Invoke-CppLogin "http" "05_http_login.json" $extra
    $reports["http_login"] = Get-ReportData $r
    return $reports
}

function Run-TcpScenario {
    # Pre-test warmup
    Invoke-PreTestWarmup -ScenarioName "tcp" -Iterations $WarmupIterations

    $r = Invoke-CppLogin "tcp" "tcp_login.json"
    return @{ "tcp" = (Get-ReportData $r) }
}

function Run-QuicScenario {
    # Pre-test warmup: critical for fair comparison with TCP
    # QUIC tests often run first and suffer from cold cache
    Invoke-PreTestWarmup -ScenarioName "quic" -Iterations $WarmupIterations

    $r = Invoke-CppLogin "quic" "quic_login.json"
    return @{ "quic" = (Get-ReportData $r) }
}

function Run-HttpScenario {
    # Pre-test warmup
    Invoke-PreTestWarmup -ScenarioName "http" -Iterations $WarmupIterations

    $r = Invoke-CppLogin "http" "http_login.json"
    return @{ "http" = (Get-ReportData $r) }
}

function Run-FriendScenario {
    $r = Invoke-PythonScript "tcp_friendship_load.py" "06_tcp_friendship.json" -UseAccountsCsv
    return @{ "friend" = (Get-ReportData $r) }
}

function Run-GroupScenario {
    $r = Invoke-PythonScript "tcp_group_ops_load.py" "07_tcp_group_ops.json" -UseAccountsCsv
    return @{ "group" = (Get-ReportData $r) }
}

function Run-HistoryScenario {
    $r = Invoke-PythonScript "tcp_history_ack_load.py" "08_tcp_history_ack.json" -UseAccountsCsv
    return @{ "history" = (Get-ReportData $r) }
}

function Run-MediaScenario {
    $r = Invoke-PythonScript "media_load.py" "09_media_load.json" -UseAccountsCsv
    return @{ "media" = (Get-ReportData $r) }
}

function Run-CallScenario {
    $r = Invoke-PythonScript "tcp_call_invite_load.py" "10_tcp_call_invite.json" -UseAccountsCsv
    return @{ "call" = (Get-ReportData $r) }
}

function Run-PostgreSqlScenario {
    $r = Invoke-PythonScript "postgresql_capacity_load.py" "11_postgresql_capacity.json" -UseAccountsCsv
    return @{ "postgresql" = (Get-ReportData $r) }
}

function Run-RedisScenario {
    $r = Invoke-PythonScript "redis_capacity_load.py" "12_redis_capacity.json"
    return @{ "redis" = (Get-ReportData $r) }
}

# ---------------------------------------------------------------------------
# Main dispatch
# ---------------------------------------------------------------------------
Write-Host "[INFO] MemoChat load suite start"
Write-Host "[INFO] Config: $Config"
Write-Host "[INFO] Scenario: $Scenario"
if ($AccountsCsv) { Write-Host "[INFO] Accounts CSV: $AccountsCsv" }
Write-Host "[INFO] Suite output: $suiteDir"

$allReports = @{}

switch ($Scenario) {
    "auth"       { $allReports += Run-AuthScenario }
    "login"      { $allReports += Run-LoginScenario }
    "tcp"        { $allReports += Run-TcpScenario }
    "quic"       { $allReports += Run-QuicScenario }
    "http"       { $allReports += Run-HttpScenario }
    "friend"     { $allReports += Run-FriendScenario }
    "group"      { $allReports += Run-GroupScenario }
    "history"    { $allReports += Run-HistoryScenario }
    "media"      { $allReports += Run-MediaScenario }
    "call"       { $allReports += Run-CallScenario }
    "postgresql" { $allReports += Run-PostgreSqlScenario }
    "mysql"      { $allReports += Run-PostgreSqlScenario }
    "redis"      { $allReports += Run-RedisScenario }
    "all" {
        $allReports += Run-AuthScenario
        $allReports += Run-LoginScenario
        $allReports += Run-FriendScenario
        $allReports += Run-GroupScenario
        $allReports += Run-HistoryScenario
        $allReports += Run-MediaScenario
        $allReports += Run-CallScenario
        $allReports += Run-PostgreSqlScenario
        $allReports += Run-RedisScenario
    }
}

# ---------------------------------------------------------------------------
# Build suite summary
# ---------------------------------------------------------------------------
$totalSuccess = 0
$totalFailed  = 0

$breakdown = @()
foreach ($key in $allReports.Keys) {
    $rep = $allReports[$key]
    if (-not $rep) { continue }

    $s = if ($rep.summary) {
        $rep.summary
    } else {
        # C++ binary report schema
        @{
            success = $rep.success
            failed  = $rep.failed
            success_rate = $rep.success_rate
            throughput_rps = $rep.rps
            latency_ms = @{ p95 = $rep.latency_ms.p95 }
        }
    }

    $totalSuccess += [int]$s.success
    $totalFailed  += [int]$s.failed

    $p95 = if ($s.latency_ms) {
        $s.latency_ms.p95
    } else { 0 }

    $breakdown += [ordered]@{
        scenario      = $key
        test          = if ($rep.test) { $rep.test } else { $key }
        success       = [int]$s.success
        failed        = [int]$s.failed
        success_rate  = [double]$s.success_rate
        throughput_rps = [double]$s.throughput_rps
        p95_ms        = [double]$p95
        top_errors    = if ($rep.top_errors) { $rep.top_errors } else { @() }
    }
}

$suiteSummary = [ordered]@{
    scenario   = $Scenario
    started_at = $timestamp
    config     = (Resolve-Path $Config).Path
    suite_dir  = $suiteDir
    totals     = [ordered]@{
        tests   = $breakdown.Count
        success = $totalSuccess
        failed  = $totalFailed
    }
    breakdown  = $breakdown
}

$summaryPath = Join-Path $suiteDir "suite_summary.json"
$suiteSummary | ConvertTo-Json -Depth 10 | Set-Content -Path $summaryPath -Encoding UTF8

Write-Host "`n[DONE] Suite summary: $summaryPath"
Write-Host "[DONE] Suite directory: $suiteDir"
