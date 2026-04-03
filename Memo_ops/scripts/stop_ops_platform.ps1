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
        $pidValue = Get-Content $_.FullName -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($pidValue) {
            $process = Get-Process -Id ([int]$pidValue) -ErrorAction SilentlyContinue
            $name = if ($process) { $process.ProcessName } else { "<unknown>" }
            Stop-Process -Id ([int]$pidValue) -Force -ErrorAction SilentlyContinue
            Write-Host "Stopped: $name (PID $pidValue)" -ForegroundColor Cyan
        }
        Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
    }
} else {
    Write-Host "No PID directory found at: $PidRoot" -ForegroundColor Yellow
}
