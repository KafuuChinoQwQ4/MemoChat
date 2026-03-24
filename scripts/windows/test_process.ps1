param(
    [string]$ExeName = "GateServer.exe"
)

$proc = Get-CimInstance Win32_Process -Filter "Name='$ExeName'" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) {
    Write-Error "Process $ExeName not found"
    exit 1
}
$path = $proc.ExecutablePath
if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path $path)) {
    Write-Error "Executable path invalid"
    exit 1
}
$item = Get-Item $path
Write-Output "[INFO] Running ${ExeName}: $($item.FullName)"
