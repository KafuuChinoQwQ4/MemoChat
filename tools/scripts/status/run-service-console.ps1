param(
    [Parameter(Mandatory = $true)]
    [string]$ServiceDir,

    [Parameter(Mandatory = $true)]
    [string]$ExeName,

    [Parameter(Mandatory = $true)]
    [string]$ServiceName,

    [Parameter(Mandatory = $true)]
    [string]$ConfigName,

    [Parameter(Mandatory = $true)]
    [string]$LogOut,

    [Parameter(Mandatory = $true)]
    [string]$LogErr
)

$ErrorActionPreference = 'Stop'

$exePath = Join-Path -Path $ServiceDir -ChildPath $ExeName
if (-not (Test-Path -LiteralPath $exePath)) {
    Write-Host "[runner] $ServiceName executable not found: $exePath" -ForegroundColor Red
    return
}

$logDir = Split-Path -Path $LogOut -Parent
if ($logDir) {
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
}

Set-Content -LiteralPath $LogOut -Value '' -Encoding UTF8
Set-Content -LiteralPath $LogErr -Value '' -Encoding UTF8

$env:MEMOCHAT_ENABLE_KAFKA = '1'
$env:MEMOCHAT_ENABLE_RABBITMQ = '1'

Set-Location -LiteralPath $ServiceDir

Write-Host "============================================================"
Write-Host "  $ServiceName"
Write-Host "  exe:    $exePath"
Write-Host "  config: $ConfigName"
Write-Host "  log:    $LogOut"
Write-Host "============================================================"

try {
    $cmdLine = "`"$exePath`" --config `"$ConfigName`" 2>&1"
    & $env:ComSpec /d /s /c $cmdLine | ForEach-Object {
        $line = $_
        Write-Host $line
        Add-Content -LiteralPath $LogOut -Value $line -Encoding UTF8
    }
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
} catch {
    $exitCode = 1
    $message = "[runner] $ServiceName failed: $($_.Exception.Message)"
    Write-Host $message -ForegroundColor Red
    Add-Content -LiteralPath $LogErr -Value $message -Encoding UTF8
    Add-Content -LiteralPath $LogOut -Value $message -Encoding UTF8
}

$exitMessage = "[runner] $ServiceName exited with code $exitCode"
if ($exitCode -eq 0) {
    Write-Host $exitMessage
} else {
    Write-Host $exitMessage -ForegroundColor Red
    Add-Content -LiteralPath $LogErr -Value $exitMessage -Encoding UTF8
    Add-Content -LiteralPath $LogOut -Value $exitMessage -Encoding UTF8
}
