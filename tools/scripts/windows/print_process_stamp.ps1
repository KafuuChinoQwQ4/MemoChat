param(
    [string]$ExeName = ""
)

if ([string]::IsNullOrWhiteSpace($ExeName)) {
    Write-Error "ExeName is required"
    exit 1
}

$proc = Get-CimInstance Win32_Process -Filter "Name='$ExeName'" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) {
    exit 1
}
$path = $proc.ExecutablePath
if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path $path)) {
    exit 1
}
$item = Get-Item $path
Write-Output "[INFO] Running $ExeName`: $($item.FullName) ($($item.Length) bytes, $($item.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss')))"
