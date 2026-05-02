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

function Find-Binary {
    param([string]$File, [string[]]$SearchDirs)
    foreach ($dir in $SearchDirs) {
        if (-not [string]::IsNullOrWhiteSpace($dir) -and (Test-Path $dir)) {
            # Direct search in dir
            $path = Join-Path $dir $File
            if (Test-Path $path) {
                return Get-Item $path
            }
            # Search inside bin/ subdir
            $binPath = Join-Path (Join-Path $dir 'bin') $File
            if (Test-Path $binPath) {
                return Get-Item $binPath
            }
            # Search inside bin/Release/ subdir
            $releasePath = Join-Path (Join-Path $dir 'bin\Release') $File
            if (Test-Path $releasePath) {
                return Get-Item $releasePath
            }
        }
    }
    return $null
}

foreach ($target in $targets) {
    # Build extended search paths (add bin/ variant for PrimaryDir/FallbackDir)
    $searchDirs = @($target.SubDirs | ForEach-Object { Join-Path $MemoOpsDir $_ })
    $searchDirs += @(
        $PrimaryDir,
        (Join-Path $PrimaryDir 'bin'),
        (Join-Path $PrimaryDir 'bin\Release'),
        $FallbackDir,
        (Join-Path $FallbackDir 'bin'),
        (Join-Path $FallbackDir 'bin\Release')
    )

    $best = Find-Binary -File $target.File -SearchDirs $searchDirs

    if (-not $best) {
        throw "Missing $($target.File) under $PrimaryDir, $FallbackDir, or MemoOps subdirs"
    }

    Write-Host ("[INFO] Selected {0}: {1} [{2}]" -f $target.File, $best.FullName, $best.LastWriteTimeUtc.ToString('yyyy-MM-dd HH:mm:ss'))
    Write-Output ("set {0}={1}" -f $target.Env, $best.FullName)
}
