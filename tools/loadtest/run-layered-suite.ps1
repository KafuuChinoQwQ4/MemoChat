param(
    [string]$ConfigPath = "",
    [string]$Cases = "default",
    [int]$Total = 200,
    [int]$Concurrency = 20,
    [int]$DbTotal = 0,
    [int]$DbConcurrency = 0,
    [string]$ReportDir = "",
    [int]$TimeoutSec = 600,
    [switch]$Heavy,
    [switch]$IncludeDeepAi
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
    return (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

$repoRoot = Resolve-RepoRoot
$runner = Join-Path $repoRoot "tools\loadtest\python-loadtest\run_layered_suite.py"
if (-not $ConfigPath) {
    $ConfigPath = Join-Path $repoRoot "tools\loadtest\python-loadtest\config.benchmark.json"
}

$argsList = @(
    $runner,
    "--config", $ConfigPath,
    "--cases", $Cases,
    "--total", [string]$Total,
    "--concurrency", [string]$Concurrency,
    "--timeout-sec", [string]$TimeoutSec
)
if ($DbTotal -gt 0) {
    $argsList += @("--db-total", [string]$DbTotal)
}
if ($DbConcurrency -gt 0) {
    $argsList += @("--db-concurrency", [string]$DbConcurrency)
}
if ($ReportDir) {
    $argsList += @("--report-dir", $ReportDir)
}
if ($Heavy) {
    $argsList += "--heavy"
}
if ($IncludeDeepAi) {
    $argsList += "--include-deep-ai"
}

python @argsList
exit $LASTEXITCODE
