# Redirect: use local-loadtest-cpp/run_suite.ps1
# This directory is kept for backwards compatibility only.
# All tools and documentation have moved to local-loadtest-cpp/
#
# New usage:
#   cd local-loadtest-cpp
#   .\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario all
#
# Direct C++ binary:
#   .\build\Release\memochat_loadtest.exe --scenario tcp --config config.json

$ErrorActionPreference = "Stop"
$script_dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$new_script = Join-Path $script_dir "..\local-loadtest-cpp\run_suite.ps1"

if (-not (Test-Path $new_script)) {
    Write-Error "local-loadtest-cpp\run_suite.ps1 not found. Please run from local-loadtest-cpp directly."
}

Write-Host "[INFO] This script has moved. Invoking local-loadtest-cpp\run_suite.ps1 ..."
Write-Host "[INFO] New location: $new_script"
Write-Host ""

& $new_script @args
