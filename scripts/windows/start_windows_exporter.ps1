param(
    [string]$Version = "0.27.2",
    [string]$InstallRoot = "D:\\docker-data\\memochat\\windows-exporter",
    [int]$Port = 9182
)

$ErrorActionPreference = "Stop"

$exePath = Join-Path $InstallRoot "windows_exporter.exe"
$zipPath = Join-Path $InstallRoot ("windows_exporter-{0}.zip" -f $Version)
$extractRoot = Join-Path $InstallRoot ("windows_exporter-{0}" -f $Version)
$url = "https://github.com/prometheus-community/windows_exporter/releases/download/v{0}/windows_exporter-{0}-amd64.exe" -f $Version

New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null

$existing = Get-CimInstance Win32_Process | Where-Object {
    $_.Name -ieq "windows_exporter.exe" -and $_.CommandLine -match "--web.listen-address=:$Port"
}
if ($existing) {
    Write-Host "windows_exporter already running on port $Port."
    exit 0
}

if (-not (Test-Path $exePath)) {
    Invoke-WebRequest -Uri $url -OutFile $exePath
}

$args = @(
    "--web.listen-address=:$Port",
    "--collectors.enabled=cpu,cs,logical_disk,memory,net,os,system"
)

Start-Process -FilePath $exePath -ArgumentList $args -WindowStyle Hidden
Write-Host "windows_exporter started on http://127.0.0.1:$Port/metrics"
