param(
    [switch]$NoClient,
    [switch]$Background,
    [switch]$SkipBuild,
    [string]$ConfigurePreset = "msvc2022-ops",
    [string]$BuildPreset = "msvc2022-ops",
    [string]$RepoRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$root = Join-Path $RepoRoot "Memo_ops"
$python = "python"
$requirements = Join-Path $root "requirements.txt"
$opsServerConfig = Join-Path $root "config\opsserver.yaml"
$opsCollectorConfig = Join-Path $root "config\opscollector.yaml"
$qmlDir = Join-Path $root "client\MemoOps-qml\qml"
$manualStartLogDir = Join-Path $root "artifacts\logs\manual-start"
$pidRoot = Join-Path $root "artifacts\pids"
$clientExe = Join-Path $RepoRoot "build\bin\Release\MemoOpsQml.exe"

New-Item -ItemType Directory -Force -Path $manualStartLogDir | Out-Null
New-Item -ItemType Directory -Force -Path $pidRoot | Out-Null

function Resolve-Windeployqt {
    $cmd = Get-Command windeployqt -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "D:\qt\6.8.3\msvc2022_64\bin\windeployqt.exe",
        "C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Write-Banner {
    param([string]$Msg)
    Write-Host ""
    Write-Host ("=" * 60) -ForegroundColor Cyan
    Write-Host $Msg -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor Cyan
}

function Test-BackendReady {
    param([string]$Url, [int]$TimeoutSec = 30, [int]$IntervalSec = 2)
    $elapsed = 0
    while ($elapsed -lt $TimeoutSec) {
        try {
            $resp = Invoke-WebRequest -Uri $Url -Method GET -TimeoutSec 5 -UseBasicParsing -ErrorAction SilentlyContinue
            if ($resp.StatusCode -eq 200) { return $true }
        } catch {}
        Start-Sleep -Seconds $IntervalSec
        $elapsed += $IntervalSec
    }
    return $false
}

# ─── 0. Python 依赖 & schema ───────────────────────────────────────────────
Write-Banner "Installing Python dependencies and initializing schema..."
Push-Location $RepoRoot
try {
    & $python -m pip install -r $requirements -q
    $env:PYTHONPATH = $RepoRoot
    & $python "$root\scripts\init_memo_ops_schema.py"
}
finally {
    Pop-Location
}

# ─── 1. Backend ─────────────────────────────────────────────────────────────
Write-Banner "Starting MemoOps backend services..."

$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"

$opsServerScriptPath = Join-Path $env:TEMP "memochat_ops_server_$PID.py"
$opsCollectorScriptPath = Join-Path $env:TEMP "memochat_ops_collector_$PID.py"

$opsServerScript = @"
import sys
sys.path.insert(0, r'$RepoRoot')
from Memo_ops.server.ops_server.main import main
sys.argv = ['ops_server', '--config', r'$opsServerConfig']
main()
"@

$opsCollectorScript = @"
import sys
sys.path.insert(0, r'$RepoRoot')
from Memo_ops.server.ops_collector.main import main
sys.argv = ['ops_collector', '--config', r'$opsCollectorConfig']
main()
"@

$opsServerScript | Out-File -FilePath $opsServerScriptPath -Encoding utf8
$opsCollectorScript | Out-File -FilePath $opsCollectorScriptPath -Encoding utf8

$serverStartParams = @{
    FilePath         = $python
    ArgumentList     = $opsServerScriptPath
    PassThru         = $true
    RedirectStandardOutput = Join-Path $manualStartLogDir "MemoOpsServer.out.log"
    RedirectStandardError  = Join-Path $manualStartLogDir "MemoOpsServer.err.log"
    WorkingDirectory = $RepoRoot
}
$collectorStartParams = @{
    FilePath         = $python
    ArgumentList     = $opsCollectorScriptPath
    PassThru         = $true
    RedirectStandardOutput = Join-Path $manualStartLogDir "MemoOpsCollector.out.log"
    RedirectStandardError  = Join-Path $manualStartLogDir "MemoOpsCollector.err.log"
    WorkingDirectory = $RepoRoot
}

if (-not $Background) {
    $serverStartParams.WindowStyle    = "Normal"
    $collectorStartParams.WindowStyle = "Normal"
    $serverStartParams.Remove("RedirectStandardOutput")
    $serverStartParams.Remove("RedirectStandardError")
    $collectorStartParams.Remove("RedirectStandardOutput")
    $collectorStartParams.Remove("RedirectStandardError")
}

Write-Host "  Launching MemoOpsServer..."
$opsServerProc = Start-Process @serverStartParams
Write-Host "  Launching MemoOpsCollector..."
$opsCollectorProc = Start-Process @collectorStartParams

Set-Content -Path (Join-Path $pidRoot "MemoOpsServer.pid")    -Value $opsServerProc.Id    -Encoding ascii
Set-Content -Path (Join-Path $pidRoot "MemoOpsCollector.pid") -Value $opsCollectorProc.Id -Encoding ascii

Write-Host "  Waiting for MemoOpsServer to be ready (http://127.0.0.1:18080)..."
$serverReady = Test-BackendReady -Url "http://127.0.0.1:18080/healthz"
if (-not $serverReady) {
    Write-Warning "MemoOpsServer did not respond within 30s. Check artifacts/logs/manual-start/MemoOpsServer.*.log"
}

# ─── 2. Client ──────────────────────────────────────────────────────────────
if (-not $NoClient) {
    Write-Banner "Building MemoOpsQml client..."

    # Only run cmake configure if the build dir doesn't exist yet (first time).
    # If already configured, cmake configure would re-run unnecessarily.
    $buildDir = Join-Path $RepoRoot "build"
    if (-not (Test-Path $buildDir) -and -not $SkipBuild) {
        Write-Host "  Configuring build (first time)..."
        Push-Location $RepoRoot
        try {
            cmake --preset $ConfigurePreset *>$null
        }
        finally {
            Pop-Location
        }
    }

    if (-not $SkipBuild) {
        Write-Host "  Building MemoOpsQml (Release)..."
        Push-Location $RepoRoot
        try {
            cmake --build --preset $BuildPreset --target MemoOpsQml
        }
        finally {
            Pop-Location
        }
    }

    if (Test-Path $clientExe) {
        Write-Host "  Deploying Qt runtime via windeployqt..."
        $windeployqt = Resolve-Windeployqt
        if ($windeployqt) {
            & $windeployqt --release --compiler-runtime --qmldir $qmlDir $clientExe *>$null
        } else {
            Write-Warning "windeployqt not found — MemoOpsQml may fail without Qt runtime deployment."
        }

        Write-Host "  Launching MemoOpsQml..."
        $clientStartParams = @{
            FilePath         = $clientExe
            WorkingDirectory = (Split-Path $clientExe)
            PassThru         = $true
        }
        if ($Background) {
            $clientStartParams.RedirectStandardOutput = Join-Path $manualStartLogDir "MemoOpsQml.out.log"
            $clientStartParams.RedirectStandardError  = Join-Path $manualStartLogDir "MemoOpsQml.err.log"
        }
        $clientProc = Start-Process @clientStartParams
        Set-Content -Path (Join-Path $pidRoot "MemoOpsQml.pid") -Value $clientProc.Id -Encoding ascii
    } else {
        Write-Warning "MemoOpsQml.exe not found at $clientExe. Build may have failed."
        Write-Warning "Try: cmake --preset msvc2022-ops && cmake --build --preset msvc2022-ops --target MemoOpsQml"
    }
}

# ─── 3. Summary ─────────────────────────────────────────────────────────────
Write-Banner "MemoOps platform is starting up"
Write-Host "  Backend:  http://127.0.0.1:18080  (health: /healthz)"
Write-Host "  Frontend: $clientExe"
Write-Host "  Logs:     $manualStartLogDir"
Write-Host "  PIDs:     $pidRoot"
Write-Host ""
Write-Host "Stop: .\stop_ops_platform.ps1" -ForegroundColor Yellow
