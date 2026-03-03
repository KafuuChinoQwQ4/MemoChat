param(
    [string]$Config = ".\config.json",
    [string]$AccountsCsv = "",
    [switch]$SkipHttp,
    [switch]$SkipTcp
)

$ErrorActionPreference = "Stop"
Set-Location -Path $PSScriptRoot

if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    throw "python not found in PATH"
}

Write-Host "[INFO] MemoChat load suite start"
Write-Host "[INFO] Config: $Config"
if ($AccountsCsv) { Write-Host "[INFO] Accounts CSV: $AccountsCsv" }

if (-not $SkipHttp) {
    Write-Host "[STEP] HTTP login load"
    $args = @(".\http_login_load.py", "--config", $Config)
    if ($AccountsCsv) { $args += @("--accounts-csv", $AccountsCsv) }
    python @args
}

if (-not $SkipTcp) {
    Write-Host "[STEP] TCP login load"
    $args = @(".\tcp_login_load.py", "--config", $Config)
    if ($AccountsCsv) { $args += @("--accounts-csv", $AccountsCsv) }
    python @args
}

Write-Host "[DONE] Reports folder: $PSScriptRoot\reports"
