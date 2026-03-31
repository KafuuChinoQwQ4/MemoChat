param(
    [switch]$NoClient,
    [switch]$Background,
    [switch]$SkipBuild,
    [string]$PidRoot = "",
    [string]$ConfigurePreset = "msvc2022-memo-ops",
    [string]$BuildPreset = "msvc2022-memo-ops-release"
)

$ErrorActionPreference = "Stop"
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"
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

    $opsServerScriptPath = Join-Path $env:TEMP "memochat_ops_server_$$.py"
    $opsCollectorScriptPath = Join-Path $env:TEMP "memochat_ops_collector_$$.py"

    $opsServerScript = @"
import sys
sys.path.insert(0, r'$repoRoot')
from Memo_ops.server.ops_server.main import main
sys.argv = ['ops_server', '--config', r'$opsServerConfig']
main()
"@
    $opsCollectorScript = @"
import sys
sys.path.insert(0, r'$repoRoot')
from Memo_ops.server.ops_collector.main import main
sys.argv = ['ops_collector', '--config', r'$opsCollectorConfig']
main()
"@

    $opsServerScript | Out-File -FilePath $opsServerScriptPath -Encoding utf8
    $opsCollectorScript | Out-File -FilePath $opsCollectorScriptPath -Encoding utf8

    $serverStartParams = @{
        FilePath = $python
        ArgumentList = $opsServerScriptPath
        PassThru = $true
        RedirectStandardOutput = Join-Path $manualStartLogDir "MemoOpsServer.out.log"
        RedirectStandardError = Join-Path $manualStartLogDir "MemoOpsServer.err.log"
        WorkingDirectory = $repoRoot
    }

    $collectorStartParams = @{
        FilePath = $python
        ArgumentList = $opsCollectorScriptPath
        PassThru = $true
        RedirectStandardOutput = Join-Path $manualStartLogDir "MemoOpsCollector.out.log"
        RedirectStandardError = Join-Path $manualStartLogDir "MemoOpsCollector.err.log"
        WorkingDirectory = $repoRoot
    }

    if (-not $Background) {
        $serverStartParams.WindowStyle = "Normal"
        $collectorStartParams.WindowStyle = "Normal"
        $serverStartParams.Remove("RedirectStandardOutput")
        $serverStartParams.Remove("RedirectStandardError")
        $collectorStartParams.Remove("RedirectStandardOutput")
        $collectorStartParams.Remove("RedirectStandardError")
    }

    $opsServerProc = Start-Process @serverStartParams
    $opsCollectorProc = Start-Process @collectorStartParams
    Set-Content -Path (Join-Path $PidRoot "MemoOpsServer.pid") -Value $opsServerProc.Id -Encoding ascii
    Set-Content -Path (Join-Path $PidRoot "MemoOpsCollector.pid") -Value $opsCollectorProc.Id -Encoding ascii

    if (-not $NoClient) {
        try {
            Push-Location $root
            if (-not $SkipBuild) {
                cmake --preset $ConfigurePreset | Out-Null
                cmake --build --preset $BuildPreset --target MemoOpsQml | Out-Null
            }
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
    if (Test-Path $opsServerScriptPath) { Remove-Item $opsServerScriptPath -ErrorAction SilentlyContinue }
    if (Test-Path $opsCollectorScriptPath) { Remove-Item $opsCollectorScriptPath -ErrorAction SilentlyContinue }
}
