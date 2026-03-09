param(
    [switch]$NoClient,
    [string]$ConfigurePreset = "msvc2022-memo-ops",
    [string]$BuildPreset = "msvc2022-memo-ops-release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $root
$python = "python"
$requirements = Join-Path $root "requirements.txt"
$opsServerConfig = Join-Path $root "config\opsserver.yaml"
$opsCollectorConfig = Join-Path $root "config\opscollector.yaml"
$qmlDir = Join-Path $root "client\MemoOps-qml\qml"

function Resolve-Windeployqt {
    $command = Get-Command windeployqt -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidates = @(
        "D:\qt\6.8.3\msvc2022_64\bin\windeployqt.exe",
        "C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

Push-Location $repoRoot
try {
    & $python -m pip install -r $requirements
    $env:PYTHONPATH = $repoRoot
    & $python "$root\scripts\init_memo_ops_schema.py"

    Start-Process powershell -ArgumentList @(
        "-NoExit",
        "-Command",
        "Set-Location '$repoRoot'; `$env:PYTHONPATH='$repoRoot'; python -m Memo_ops.server.ops_server.main --config '$opsServerConfig'"
    ) | Out-Null

    Start-Process powershell -ArgumentList @(
        "-NoExit",
        "-Command",
        "Set-Location '$repoRoot'; `$env:PYTHONPATH='$repoRoot'; python -m Memo_ops.server.ops_collector.main --config '$opsCollectorConfig'"
    ) | Out-Null

    if (-not $NoClient) {
        try {
            Push-Location $root
            cmake --preset $ConfigurePreset | Out-Null
            cmake --build --preset $BuildPreset --target MemoOpsQml | Out-Null
            Pop-Location
            $clientExe = Join-Path $repoRoot "build-memo-ops\bin\Release\MemoOpsQml.exe"
            if (Test-Path $clientExe) {
                $clientDir = Split-Path -Parent $clientExe
                $windeployqt = Resolve-Windeployqt
                if ($windeployqt) {
                    & $windeployqt --release --compiler-runtime --qmldir $qmlDir $clientExe | Out-Null
                } else {
                    Write-Warning "windeployqt was not found; MemoOpsQml may fail to start without Qt runtime deployment."
                }
                Start-Process -FilePath $clientExe -WorkingDirectory $clientDir | Out-Null
            } else {
                Write-Warning "MemoOpsQml.exe was not found at $clientExe"
            }
        }
        catch {
            if ((Get-Location).Path -eq $root) {
                Pop-Location
            }
            Write-Warning "MemoOpsQml build/launch skipped: $($_.Exception.Message)"
        }
    }
}
finally {
    Pop-Location
}
