param(
    [string]$ExeName = ""
)

if ([string]::IsNullOrWhiteSpace($ExeName)) {
    exit 1
}

$count = (Get-Process -Name $ExeName -ErrorAction SilentlyContinue | Measure-Object).Count
Write-Output "[INFO] Running ${ExeName}.exe instances: $count"
