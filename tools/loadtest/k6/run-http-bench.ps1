param(
    [string]$ConfigPath = "",
    [ValidateSet("login", "health")]
    [string]$Scenario = "login",
    [int]$Total = 100,
    [int]$Concurrency = 10,
    [string]$SummaryPath = "",
    [switch]$UseDocker,
    [string]$K6Image = "grafana/k6:0.50.0"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    try {
        $root = git rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and $root) {
            return (Resolve-Path $root.Trim()).Path
        }
    } catch {
    }
    return (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
}

function Convert-ToDockerPath {
    param([string]$Path)
    $resolved = (Resolve-Path $Path).Path
    $drive = $resolved.Substring(0, 1).ToLowerInvariant()
    $rest = $resolved.Substring(2).Replace("\", "/")
    return "/$drive$rest"
}

function Convert-ToWorkPath {
    param(
        [string]$Path,
        [string]$Root
    )
    if (Test-Path -LiteralPath $Path) {
        $resolved = (Resolve-Path $Path).Path
    } else {
        $parent = Split-Path -Parent $Path
        $leaf = Split-Path -Leaf $Path
        if (-not $parent) {
            $parent = Get-Location
        }
        $resolved = (Join-Path (Resolve-Path $parent).Path $leaf)
    }
    $rootResolved = (Resolve-Path $Root).Path.TrimEnd("\")
    if (-not $resolved.StartsWith($rootResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path must be under repo root for Docker k6: $resolved"
    }
    $relative = $resolved.Substring($rootResolved.Length).TrimStart("\").Replace("\", "/")
    return "/work/$relative"
}

$repoRoot = Resolve-RepoRoot
if (-not $ConfigPath) {
    $ConfigPath = Join-Path $repoRoot "tools\loadtest\python-loadtest\config.benchmark.json"
}
$configPathResolved = (Resolve-Path $ConfigPath).Path
$config = Get-Content -LiteralPath $configPathResolved -Raw | ConvertFrom-Json
$configDir = Split-Path -Parent $configPathResolved

$reportDir = if ($config.report_dir) {
    $candidate = [string]$config.report_dir
    if ([System.IO.Path]::IsPathRooted($candidate)) {
        $candidate
    } else {
        Join-Path $configDir $candidate
    }
} else {
    Join-Path $repoRoot "infra\Memo_ops\artifacts\loadtest\runtime\reports"
}
$reportDir = (New-Item -ItemType Directory -Force -Path $reportDir).FullName
if (-not $SummaryPath) {
    $SummaryPath = Join-Path $reportDir ("k6_http_{0}_{1}.json" -f $Scenario, (Get-Date -Format "yyyyMMdd_HHmmss"))
}
$summaryDir = Split-Path -Parent $SummaryPath
New-Item -ItemType Directory -Force -Path $summaryDir | Out-Null

$accountsCsv = if ($config.accounts_csv) {
    $candidate = [string]$config.accounts_csv
    if ([System.IO.Path]::IsPathRooted($candidate)) {
        $candidate
    } else {
        Join-Path $configDir $candidate
    }
} else {
    Join-Path $repoRoot "infra\Memo_ops\artifacts\loadtest\runtime\accounts\accounts.local.csv"
}
$accountsCsv = (Resolve-Path $accountsCsv).Path

$gateUrls = @()
if ($config.gate_urls) {
    foreach ($url in $config.gate_urls) {
        $gateUrls += [string]$url
    }
} elseif ($config.gate_url) {
    $gateUrls += [string]$config.gate_url
} else {
    $gateUrls += "http://127.0.0.1:8080"
}

$scriptPath = Join-Path $PSScriptRoot "http-login.js"
if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "k6 script not found: $scriptPath"
}

$env:GATE_URLS = ($gateUrls -join ",")
$env:LOGIN_PATH = if ($config.login_path) { [string]$config.login_path } else { "/user_login" }
$env:CLIENT_VER = if ($config.client_ver) { [string]$config.client_ver } else { "3.0.0" }
$env:USE_XOR_PASSWD = if ($null -ne $config.use_xor_passwd) { [string]$config.use_xor_passwd } else { "true" }
$env:ACCOUNTS_CSV = $accountsCsv
$env:TOTAL = [string]$Total
$env:CONCURRENCY = [string]$Concurrency
$env:SCENARIO = $Scenario
$env:SUMMARY_PATH = $SummaryPath
$env:HTTP_TIMEOUT = if ($config.http_timeout_sec) { "$([int]$config.http_timeout_sec)s" } else { "8s" }

$localK6 = Get-Command k6 -ErrorAction SilentlyContinue
if ($localK6 -and -not $UseDocker) {
    & $localK6.Source run $scriptPath
    exit $LASTEXITCODE
}

$dockerRepoRoot = Convert-ToDockerPath $repoRoot
$dockerSummary = Convert-ToWorkPath -Path $SummaryPath -Root $repoRoot
$dockerAccounts = Convert-ToWorkPath -Path $accountsCsv -Root $repoRoot
$dockerScript = "/work/tools/loadtest/k6/http-login.js"
$dockerGateUrls = ($gateUrls | ForEach-Object {
    $_ -replace "127\.0\.0\.1", "host.docker.internal" -replace "localhost", "host.docker.internal"
}) -join ","

docker run --rm `
    -v "${dockerRepoRoot}:/work" `
    -e "GATE_URLS=$dockerGateUrls" `
    -e "LOGIN_PATH=$env:LOGIN_PATH" `
    -e "CLIENT_VER=$env:CLIENT_VER" `
    -e "USE_XOR_PASSWD=$env:USE_XOR_PASSWD" `
    -e "ACCOUNTS_CSV=$dockerAccounts" `
    -e "TOTAL=$Total" `
    -e "CONCURRENCY=$Concurrency" `
    -e "SCENARIO=$Scenario" `
    -e "SUMMARY_PATH=$dockerSummary" `
    -e "HTTP_TIMEOUT=$env:HTTP_TIMEOUT" `
    $K6Image run $dockerScript

exit $LASTEXITCODE
