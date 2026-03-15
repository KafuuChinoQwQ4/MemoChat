param(
    [switch]$NoClient,
    [switch]$Background,
    [string]$PidRoot = "",
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
$manualStartLogDir = Join-Path $root "artifacts\logs\manual-start"

if (-not $PidRoot) {
    $PidRoot = Join-Path $root "artifacts\runtime\pids"
}

New-Item -ItemType Directory -Force -Path $manualStartLogDir | Out-Null
New-Item -ItemType Directory -Force -Path $PidRoot | Out-Null

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

    $opsServerArgs = @(
        "-NoProfile",
        "-Command",
        "Set-Location '$repoRoot'; `$env:PYTHONPATH='$repoRoot'; python -m Memo_ops.server.ops_server.main --config '$opsServerConfig'"
    )
    $opsCollectorArgs = @(
        "-NoProfile",
        "-Command",
        "Set-Location '$repoRoot'; `$env:PYTHONPATH='$repoRoot'; python -m Memo_ops.server.ops_collector.main --config '$opsCollectorConfig'"
    )

    if (-not $Background) {
        $opsServerArgs = @("-NoExit") + $opsServerArgs
        $opsCollectorArgs = @("-NoExit") + $opsCollectorArgs
    }

    $serverStartParams = @{
        FilePath = "powershell"
        ArgumentList = $opsServerArgs
        PassThru = $true
    }
    $collectorStartParams = @{
        FilePath = "powershell"
        ArgumentList = $opsCollectorArgs
        PassThru = $true
    }

    if ($Background) {
        $serverStartParams.WindowStyle = "Hidden"
        $serverStartParams.RedirectStandardOutput = (Join-Path $manualStartLogDir "MemoOpsServer.out.log")
        $serverStartParams.RedirectStandardError = (Join-Path $manualStartLogDir "MemoOpsServer.err.log")
        $collectorStartParams.WindowStyle = "Hidden"
        $collectorStartParams.RedirectStandardOutput = (Join-Path $manualStartLogDir "MemoOpsCollector.out.log")
        $collectorStartParams.RedirectStandardError = (Join-Path $manualStartLogDir "MemoOpsCollector.err.log")
    }

    $opsServerProc = Start-Process @serverStartParams
    $opsCollectorProc = Start-Process @collectorStartParams
    Set-Content -Path (Join-Path $PidRoot "MemoOpsServer.pid") -Value $opsServerProc.Id -Encoding ascii
    Set-Content -Path (Join-Path $PidRoot "MemoOpsCollector.pid") -Value $opsCollectorProc.Id -Encoding ascii

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
                $clientStartParams = @{
                    FilePath = $clientExe
                    WorkingDirectory = $clientDir
                    PassThru = $true
                }
                if ($Background) {
                    $clientStartParams.WindowStyle = "Hidden"
                    $clientStartParams.RedirectStandardOutput = (Join-Path $manualStartLogDir "MemoOpsQml.out.log")
                    $clientStartParams.RedirectStandardError = (Join-Path $manualStartLogDir "MemoOpsQml.err.log")
                }
                $clientProc = Start-Process @clientStartParams
                Set-Content -Path (Join-Path $PidRoot "MemoOpsQml.pid") -Value $clientProc.Id -Encoding ascii
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
