param(
    [string]$RepoRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$InputRoot)
    if ($InputRoot -and (Test-Path $InputRoot)) {
        return (Resolve-Path $InputRoot).Path
    }
    try {
        $gitRoot = git rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and $gitRoot) {
            return $gitRoot.Trim()
        }
    } catch {
    }
    return (Get-Location).Path
}

function Test-PortInUse {
    param([int]$Port)
    $pattern = ":{0}\s" -f $Port
    $match = netstat -ano -p tcp | Select-String -Pattern $pattern
    return [bool]$match
}

function Get-IniData {
    param([string]$Path)
    $sections = @{}
    $current = ""
    foreach ($raw in Get-Content $Path) {
        $line = $raw.Trim()
        if (-not $line -or $line.StartsWith(";") -or $line.StartsWith("#")) {
            continue
        }
        if ($line -match '^\[(.+)\]$') {
            $current = $Matches[1].Trim()
            if (-not $sections.ContainsKey($current)) {
                $sections[$current] = @{}
            }
            continue
        }
        $parts = $line.Split("=", 2)
        if ($current -and $parts.Count -eq 2) {
            $sections[$current][$parts[0].Trim()] = $parts[1].Trim()
        }
    }
    return $sections
}

$root = Resolve-RepoRoot -InputRoot $RepoRoot
$statusConfig = Join-Path $root "server/StatusServer/config.ini"
if (-not (Test-Path $statusConfig)) {
    throw "Missing required config: $statusConfig"
}

$statusIni = Get-IniData -Path $statusConfig
$clusterNodes = @()
if ($statusIni.ContainsKey("Cluster") -and $statusIni["Cluster"].ContainsKey("Nodes")) {
    $clusterNodes = @(
        ($statusIni["Cluster"]["Nodes"] -split ",") |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ }
    )
}
if ($clusterNodes.Count -eq 0) {
    throw "No chat cluster nodes configured in server/StatusServer/config.ini"
}

$serverBinPrimary = Join-Path $root "build-vcpkg-server/bin/Release"
$serverBinFallback = Join-Path $root "build/bin/Release"
$binDir = if (
    (Test-Path (Join-Path $serverBinPrimary "GateServer.exe")) -and
    (Test-Path (Join-Path $serverBinPrimary "StatusServer.exe")) -and
    (Test-Path (Join-Path $serverBinPrimary "ChatServer.exe"))
) {
    $serverBinPrimary
} else {
    $serverBinFallback
}

$requiredFiles = @(
    "scripts/windows/start_test_services.bat",
    "server/GateServer/config.ini",
    "server/StatusServer/config.ini",
    "server/ChatServer/config.ini",
    "server/VarifyServer/config.json",
    "server/VarifyServer/package.json"
)

foreach ($node in $clusterNodes) {
    $requiredFiles += "server/ChatServer/$node.ini"
}

$requiredBinaries = @(
    "GateServer.exe",
    "StatusServer.exe",
    "ChatServer.exe"
)

$missing = New-Object System.Collections.Generic.List[string]
foreach ($rel in $requiredFiles) {
    $abs = Join-Path $root $rel
    if (-not (Test-Path $abs)) {
        $missing.Add("Missing file: $rel")
    }
}

foreach ($name in $requiredBinaries) {
    $abs = Join-Path $binDir $name
    if (-not (Test-Path $abs)) {
        $missing.Add("Missing binary: $($binDir.Replace($root + '\', ''))/$name")
    }
}

foreach ($cmd in @("node", "npm", "python")) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
        $missing.Add("Missing command in PATH: $cmd")
    }
}

$chatConfig = Get-IniData -Path (Join-Path $root "server/ChatServer/config.ini")
$ports = New-Object System.Collections.Generic.List[int]
foreach ($fixedPort in @(8080, 50051, 50052, 3306, 6379)) {
    $ports.Add($fixedPort)
}

$mongoEnabled = $false
if ($chatConfig.ContainsKey("Mongo") -and $chatConfig["Mongo"].ContainsKey("Enabled")) {
    $mongoEnabled = $chatConfig["Mongo"]["Enabled"] -match '^(?i:true|1|yes|on)$'
}
if ($mongoEnabled) {
    $ports.Add(27017)
}

foreach ($node in $clusterNodes) {
    if (-not $statusIni.ContainsKey($node)) {
        $missing.Add("Missing [${node}] section in server/StatusServer/config.ini")
        continue
    }
    $nodeSection = $statusIni[$node]
    foreach ($field in @("TcpPort", "RpcPort")) {
        if (-not $nodeSection.ContainsKey($field)) {
            $missing.Add("Missing ${field} in [$node]")
            continue
        }
        $ports.Add([int]$nodeSection[$field])
    }
}

Write-Host "RepoRoot: $root"
Write-Host "BinaryDir: $binDir"
Write-Host "Chat nodes: $($clusterNodes -join ', ')"
Write-Host "Port usage snapshot:"
foreach ($p in ($ports | Sort-Object -Unique)) {
    $state = if (Test-PortInUse -Port $p) { "IN_USE" } else { "FREE" }
    Write-Host ("  {0,5}: {1}" -f $p, $state)
}

if ($missing.Count -gt 0) {
    Write-Host ""
    Write-Host "Preflight failed:"
    foreach ($item in $missing) {
        Write-Host "  - $item"
    }
    exit 2
}

Write-Host ""
Write-Host "Preflight passed."
exit 0
