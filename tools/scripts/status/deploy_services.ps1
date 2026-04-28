$ErrorActionPreference = 'Stop'

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
$buildBin = Join-Path $projectRoot 'build\bin\Release'
$runtimeDir = Join-Path $projectRoot 'infra\Memo_ops\runtime\services'
$sourceRoot = Join-Path $projectRoot 'apps\server\core'
$vcpkgBin = Join-Path $projectRoot 'vcpkg_installed\x64-windows\bin'

$services = @(
    @{ Name = 'GateServer1'; Exe = 'GateServer.exe'; SourceExe = 'GateServer.exe'; Config = Join-Path $sourceRoot 'GateServer\config.ini' },
    @{ Name = 'GateServer2'; Exe = 'GateServer.exe'; SourceExe = 'GateServer.exe'; Config = Join-Path $sourceRoot 'GateServer\gate2.ini' },
    @{ Name = 'StatusServer1'; Exe = 'StatusServer.exe'; SourceExe = 'StatusServer.exe'; Config = Join-Path $sourceRoot 'StatusServer\config.ini' },
    @{ Name = 'StatusServer2'; Exe = 'StatusServer.exe'; SourceExe = 'StatusServer.exe'; Config = Join-Path $sourceRoot 'StatusServer\status2.ini' },
    @{ Name = 'chatserver1'; Exe = 'ChatServer.exe'; SourceExe = 'ChatServer.exe'; Config = Join-Path $sourceRoot 'ChatServer\chatserver1.ini' },
    @{ Name = 'chatserver2'; Exe = 'ChatServer.exe'; SourceExe = 'ChatServer.exe'; Config = Join-Path $sourceRoot 'ChatServer\chatserver2.ini' },
    @{ Name = 'chatserver3'; Exe = 'ChatServer.exe'; SourceExe = 'ChatServer.exe'; Config = Join-Path $sourceRoot 'ChatServer\chatserver3.ini' },
    @{ Name = 'chatserver4'; Exe = 'ChatServer.exe'; SourceExe = 'ChatServer.exe'; Config = Join-Path $sourceRoot 'ChatServer\chatserver4.ini' },
    @{ Name = 'chatserver5'; Exe = 'ChatServer.exe'; SourceExe = 'ChatServer.exe'; Config = Join-Path $sourceRoot 'ChatServer\chatserver5.ini' },
    @{ Name = 'chatserver6'; Exe = 'ChatServer.exe'; SourceExe = 'ChatServer.exe'; Config = Join-Path $sourceRoot 'ChatServer\chatserver6.ini' },
    @{ Name = 'AIServer'; Exe = 'AIServer.exe'; SourceExe = 'AIServer.exe'; Config = Join-Path $sourceRoot 'AIServer\config.ini' },
    @{ Name = 'VarifyServer1'; Exe = 'VarifyServer.exe'; SourceExe = 'VarifyServer.exe'; Config = Join-Path $sourceRoot 'VarifyServer\config.ini' },
    @{ Name = 'VarifyServer2'; Exe = 'VarifyServer.exe'; SourceExe = 'VarifyServer.exe'; Config = Join-Path $sourceRoot 'VarifyServer\varify2.ini' }
)

$clientApps = @(
    @{ Name = 'MemoChatQml'; Exe = 'MemoChatQml.exe'; ConfigName = 'config.ini' },
    @{ Name = 'MemoOpsQml'; Exe = 'MemoOpsQml.exe'; ConfigName = 'memoops-qml.ini' }
)

Write-Host '============================================================'
Write-Host '  Deploy services to runtime directory'
Write-Host "  PROJECT_ROOT: $projectRoot"
Write-Host "  BUILD_BIN:    $buildBin"
Write-Host "  RUNTIME_DIR:  $runtimeDir"
Write-Host '============================================================'

New-Item -ItemType Directory -Force -Path $runtimeDir | Out-Null

function Copy-RequiredFile {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Source)) {
        throw "Source not found: $Source"
    }

    $destDir = Split-Path -Path $Destination -Parent
    if ($destDir) {
        New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    Write-Host "[OK] $(Split-Path -Path $Destination -Leaf)"
}

function Copy-OptionalFile {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (Test-Path -LiteralPath $Source) {
        $destDir = Split-Path -Path $Destination -Parent
        if ($destDir) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        Write-Host "[OK] $(Split-Path -Path $Destination -Leaf)"
    } else {
        Write-Host "[WARN] Source not found: $Source"
    }
}

$cppDirs = New-Object System.Collections.Generic.List[string]

foreach ($svc in $services) {
    $svcDir = Join-Path $runtimeDir $svc.Name
    New-Item -ItemType Directory -Force -Path $svcDir | Out-Null
    $cppDirs.Add($svcDir)
    Copy-RequiredFile (Join-Path $buildBin $svc.SourceExe) (Join-Path $svcDir $svc.Exe)
    Copy-RequiredFile $svc.Config (Join-Path $svcDir 'config.ini')
}

foreach ($app in $clientApps) {
    $appDir = Join-Path $runtimeDir $app.Name
    New-Item -ItemType Directory -Force -Path $appDir | Out-Null
    Copy-OptionalFile (Join-Path $buildBin $app.Exe) (Join-Path $appDir $app.Exe)
    Copy-OptionalFile (Join-Path $buildBin $app.ConfigName) (Join-Path $appDir $app.ConfigName)
}

if (Test-Path -LiteralPath $vcpkgBin) {
    Write-Host '[DLL] Copying vcpkg DLLs...'
    $dlls = Get-ChildItem -LiteralPath $vcpkgBin -Filter '*.dll' -File
    foreach ($dir in $cppDirs) {
        foreach ($dll in $dlls) {
            Copy-Item -LiteralPath $dll.FullName -Destination $dir -Force
        }
    }
    Write-Host '[DLL] Done'
}

$gateLocalDlls = Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'GateServer') -Filter '*.dll' -File -ErrorAction SilentlyContinue
foreach ($gateDir in @('GateServer1', 'GateServer2')) {
    $targetDir = Join-Path $runtimeDir $gateDir
    foreach ($dll in $gateLocalDlls) {
        Copy-Item -LiteralPath $dll.FullName -Destination $targetDir -Force
    }
}

$required = @(
    'GateServer1\GateServer.exe',
    'GateServer2\GateServer.exe',
    'StatusServer1\StatusServer.exe',
    'StatusServer2\StatusServer.exe',
    'VarifyServer1\VarifyServer.exe',
    'VarifyServer2\VarifyServer.exe',
    'chatserver1\ChatServer.exe',
    'chatserver2\ChatServer.exe',
    'chatserver3\ChatServer.exe',
    'chatserver4\ChatServer.exe',
    'chatserver5\ChatServer.exe',
    'chatserver6\ChatServer.exe',
    'GateServer1\config.ini',
    'GateServer2\config.ini',
    'StatusServer1\config.ini',
    'StatusServer2\config.ini',
    'VarifyServer1\config.ini',
    'VarifyServer2\config.ini',
    'chatserver1\config.ini',
    'chatserver2\config.ini',
    'chatserver3\config.ini',
    'chatserver4\config.ini',
    'chatserver5\config.ini',
    'chatserver6\config.ini'
)

$missing = @()
foreach ($relative in $required) {
    $path = Join-Path $runtimeDir $relative
    if (-not (Test-Path -LiteralPath $path)) {
        $missing += $relative
    }
}

if ($missing.Count -gt 0) {
    Write-Host '[FAIL] Missing runtime files:'
    $missing | ForEach-Object { Write-Host "  $_" }
    exit 1
}

Write-Host "[SUCCESS] Runtime services deployed to $runtimeDir"

