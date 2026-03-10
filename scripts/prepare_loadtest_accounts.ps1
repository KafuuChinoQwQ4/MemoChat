param(
    [string]$RepoRoot = "",
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$InputRoot)
    if ($InputRoot -and (Test-Path $InputRoot)) {
        return (Resolve-Path $InputRoot).Path
    }
    try {
        $gitRoot = git rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and $gitRoot) {
            return $gitRoot.Trim()
        }
    } catch {
    }
    return (Get-Location).Path
}

$root = Resolve-RepoRoot -InputRoot $RepoRoot
$runtimeDir = Join-Path $root "Memo_ops/runtime/loadtest"
$target = Join-Path $runtimeDir "accounts.local.csv"
New-Item -ItemType Directory -Force -Path $runtimeDir | Out-Null

$sources = @(
    (Join-Path $root "local-loadtest/accounts.e2e.csv"),
    (Join-Path $root "local-loadtest/accounts.local.csv"),
    (Join-Path $root "local-loadtest/accounts.smoke.csv"),
    (Join-Path $root "local-loadtest/accounts.register.smoke.csv"),
    (Join-Path $root "local-loadtest/accounts.example.csv")
)

$source = $sources | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $source) {
    throw "No source accounts CSV found under local-loadtest"
}

if ((Test-Path $target) -and -not $Force) {
    Write-Host "Loadtest accounts already prepared: $target"
    exit 0
}

Copy-Item -Force:$Force -Path $source -Destination $target
Write-Host "Prepared loadtest accounts: $target"
Write-Host "Source: $source"
exit 0
