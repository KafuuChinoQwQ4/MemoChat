param(
    [string]$PidRoot = ""
)

$ErrorActionPreference = "SilentlyContinue"

if (-not $PidRoot) {
    $root = Split-Path -Parent $PSScriptRoot
    $PidRoot = Join-Path $root "artifacts\runtime\pids"
}

if (Test-Path $PidRoot) {
    Get-ChildItem -Path $PidRoot -Filter *.pid -ErrorAction SilentlyContinue | ForEach-Object {
        try {
            $pidValue = Get-Content $_.FullName -ErrorAction Stop | Select-Object -First 1
            if ($pidValue) {
                Stop-Process -Id ([int]$pidValue) -Force -ErrorAction SilentlyContinue
            }
        } catch {
        }
        Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
    }
}
