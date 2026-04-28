param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,
    [string[]]$ArgumentList = @(),
    [string]$WorkingDirectory = "",
    [string]$StdOutPath = "",
    [string]$StdErrPath = "",
    [string]$PidFile = "",
    [switch]$Hidden
)

$ErrorActionPreference = "Stop"

if (-not $WorkingDirectory) {
    $WorkingDirectory = (Get-Location).Path
}

if ($StdOutPath) {
    $stdoutDir = Split-Path -Parent $StdOutPath
    if ($stdoutDir) {
        New-Item -ItemType Directory -Force -Path $stdoutDir | Out-Null
    }
}

if ($StdErrPath) {
    $stderrDir = Split-Path -Parent $StdErrPath
    if ($stderrDir) {
        New-Item -ItemType Directory -Force -Path $stderrDir | Out-Null
    }
}

if ($PidFile) {
    $pidDir = Split-Path -Parent $PidFile
    if ($pidDir) {
        New-Item -ItemType Directory -Force -Path $pidDir | Out-Null
    }
}

$startParams = @{
    FilePath = $FilePath
    WorkingDirectory = $WorkingDirectory
    PassThru = $true
}

if ($ArgumentList.Count -gt 0) {
    $filteredArgs = @($ArgumentList | Where-Object { $_ -ne $null -and $_ -ne "" })
    if ($filteredArgs.Count -gt 0) {
        $startParams.ArgumentList = $filteredArgs
    }
}

if ($StdOutPath) {
    $startParams.RedirectStandardOutput = $StdOutPath
}

if ($StdErrPath) {
    $startParams.RedirectStandardError = $StdErrPath
}

if ($Hidden) {
    $startParams.WindowStyle = "Hidden"
}

$process = Start-Process @startParams

if ($PidFile) {
    Set-Content -Path $PidFile -Value $process.Id -Encoding ascii
}

Write-Output $process.Id
