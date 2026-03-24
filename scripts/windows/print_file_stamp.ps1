param(
    [string]$Label = "",
    [string]$Path = ""
)

if ([string]::IsNullOrWhiteSpace($Path)) {
    exit 1
}

if (-not (Test-Path $Path)) {
    exit 1
}

$item = Get-Item $Path
Write-Output "[INFO] ${Label}: $($item.FullName) ($($item.Length) bytes, $($item.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss')))"
