param(
    [Parameter(Mandatory = $true)]
    [string]$PrimaryDir,

    [Parameter(Mandatory = $true)]
    [string]$FallbackDir
)

$ErrorActionPreference = 'Stop'

$targets = @(
    @{ Env = 'GATE_EXE'; File = 'GateServer.exe' },
    @{ Env = 'STATUS_EXE'; File = 'StatusServer.exe' },
    @{ Env = 'CHAT_EXE'; File = 'ChatServer.exe' }
)

foreach ($target in $targets) {
    $candidates = @()
    foreach ($dir in @($PrimaryDir, $FallbackDir)) {
        if ([string]::IsNullOrWhiteSpace($dir)) {
            continue
        }
        $path = Join-Path $dir $target.File
        if (Test-Path $path) {
            $candidates += Get-Item $path
        }
    }

    if (-not $candidates) {
        throw "Missing $($target.File) under $PrimaryDir or $FallbackDir"
    }

    Write-Host ("[INFO] Candidate {0}: {1}" -f $target.File, (($candidates | ForEach-Object {
        "{0} [{1}]" -f $_.FullName, $_.LastWriteTimeUtc.ToString('yyyy-MM-dd HH:mm:ss')
    }) -join '; '))

    $best = $candidates | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    Write-Output ("set {0}={1}" -f $target.Env, $best.FullName)
}
