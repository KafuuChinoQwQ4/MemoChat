param(
    [Parameter(Mandatory = $true)]
    [string]$PrimaryDir,

    [Parameter(Mandatory = $true)]
    [string]$FallbackDir,

    [string]$MemoOpsDir = ""
)

$ErrorActionPreference = 'Stop'

$targets = @(
    @{ Env = 'GATE_EXE'; File = 'GateServer.exe'; SubDirs = @('GateServer', 'gate') },
    @{ Env = 'STATUS_EXE'; File = 'StatusServer.exe'; SubDirs = @('StatusServer', 'status') },
    @{ Env = 'CHAT_EXE'; File = 'ChatServer.exe'; SubDirs = @('chatserver1', 'chatserver2', 'chatserver3', 'chatserver4') }
)

foreach ($target in $targets) {
    $candidates = @()

    # Search in PrimaryDir and FallbackDir
    foreach ($dir in @($PrimaryDir, $FallbackDir)) {
        if (-not [string]::IsNullOrWhiteSpace($dir)) {
            $path = Join-Path $dir $target.File
            if (Test-Path $path) {
                $candidates += Get-Item $path
            }
        }
    }

    # Search in MemoOpsDir subdirectories
    if (-not [string]::IsNullOrWhiteSpace($MemoOpsDir) -and (Test-Path $MemoOpsDir)) {
        foreach ($subDir in $target.SubDirs) {
            $path = Join-Path $MemoOpsDir (Join-Path $subDir $target.File)
            if (Test-Path $path) {
                $candidates += Get-Item $path
            }
        }
    }

    if (-not $candidates) {
        throw "Missing $($target.File) under $PrimaryDir, $FallbackDir, or $MemoOpsDir"
    }

    Write-Host ("[INFO] Candidate {0}: {1}" -f $target.File, (($candidates | ForEach-Object {
        "{0} [{1}]" -f $_.FullName, $_.LastWriteTimeUtc.ToString('yyyy-MM-dd HH:mm:ss')
    }) -join '; '))

    $best = $candidates | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    Write-Output ("set {0}={1}" -f $target.Env, $best.FullName)
}
