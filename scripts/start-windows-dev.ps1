<#
.SYNOPSIS
    MemoChat Windows 原生方案一键启动脚本
.DESCRIPTION
    启动所有基础设施 + 后端服务 + QML 客户端
    依赖: Docker Desktop, CMake, Qt6, Node.js
.PARAMETER SkipBuild
    跳过 C++ 编译 (使用已有的 exe)
.PARAMETER SkipClient
    跳过 QML 客户端启动
.EXAMPLE
    .\start-windows-dev.ps1
    .\start-windows-dev.ps1 -SkipBuild
#>

param(
    [switch]$SkipBuild,
    [switch]$SkipClient
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ServicesDir = "$ProjectRoot\Memo_ops\runtime\services"
$env:MEMOCHAT_ENABLE_KAFKA = "1"
$env:MEMOCHAT_ENABLE_RABBITMQ = "1"

# 颜色输出
function Write-Step { param($msg) Write-Host "[步骤] $msg" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[成功] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[警告] $msg" -ForegroundColor Yellow }
function Write-Fail { param($msg) Write-Host "[错误] $msg" -ForegroundColor Red }

Write-Host @"
╔═══════════════════════════════════════════════════════════════╗
║         MemoChat Windows 原生开发环境启动器                     ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

# ============================================================
# 步骤 1: 启动基础设施
# ============================================================
Write-Step "启动 Docker 基础设施..."

$batPath = "$ProjectRoot\scripts\windows\start_test_services.bat"
if (Test-Path $batPath) {
    Start-Process -FilePath $batPath -Wait:$false
    Write-Success "基础设施启动脚本已执行"
} else {
    Write-Fail "找不到 $batPath"
    exit 1
}

# 等待基础设施就绪
Write-Step "等待基础设施就绪 (30秒)..."
Start-Sleep -Seconds 30

# 验证基础设施
Write-Step "验证 Docker 容器..."
$runningContainers = @("memochat-postgres", "memochat-redis", "memochat-mongodb", "memochat-kafka", "memochat-rabbitmq")
foreach ($name in $runningContainers) {
    $status = docker ps --filter "name=$name" --format "{{.Names}}"
    if ($status) {
        Write-Host "  ✓ $name 运行中" -ForegroundColor Green
    } else {
        Write-Warn "  ✗ $name 未运行"
    }
}

# ============================================================
# 步骤 2: 编译 C++ 服务器 (可选)
# ============================================================
if (-not $SkipBuild) {
    Write-Step "编译 C++ 服务器..."

    $buildDir = "$ProjectRoot\build"
    if (-not (Test-Path $buildDir)) {
        Write-Step "创建 build 目录并配置 CMake..."
        mkdir $buildDir -Force | Out-Null
        cd $buildDir
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    }

    Write-Step "编译..."
    cd $buildDir
    cmake --build . --config Release --parallel

    if ($LASTEXITCODE -ne 0) {
        Write-Warn "编译失败，尝试使用已有的 exe"
    } else {
        Write-Success "编译完成"
    }
} else {
    Write-Warn "跳过编译，使用已有 exe"
}

# ============================================================
# 步骤 3: 启动后端服务
# ============================================================
Write-Step "启动后端服务..."

$services = @(
    @{ Name = "GateServer";       Exe = "GateServer.exe";   Config = "$ServicesDir\GateServer\config.ini" },
    @{ Name = "StatusServer";     Exe = "StatusServer.exe"; Config = "$ServicesDir\StatusServer\config.ini" },
    @{ Name = "ChatServer-1";      Exe = "ChatServer.exe";   Config = "$ServicesDir\chatserver1\config.ini" },
    @{ Name = "ChatServer-2";      Exe = "ChatServer.exe";   Config = "$ServicesDir\chatserver2\config.ini" },
    @{ Name = "ChatServer-3";      Exe = "ChatServer.exe";   Config = "$ServicesDir\chatserver3\config.ini" },
    @{ Name = "ChatServer-4";      Exe = "ChatServer.exe";   Config = "$ServicesDir\chatserver4\config.ini" }
)

foreach ($svc in $services) {
    $svcDir = Split-Path $svc.Config -Parent
    $exePath = "$svcDir\$($svc.Exe)"
    if (Test-Path $exePath) {
        Push-Location $svcDir
        Start-Process -FilePath $exePath -ArgumentList "--config $($svc.Config)" -WindowStyle Hidden
        Pop-Location
        Write-Host "  ✓ $($svc.Name) 启动" -ForegroundColor Green
    } else {
        Write-Warn "  ✗ $($svc.Name) 未找到: $exePath"
    }
}

# 启动 VarifyServer (Node.js)
Write-Step "启动 VarifyServer..."
$vsDir = "$ProjectRoot\server\VarifyServer"
if (Test-Path $vsDir) {
    $pkgJson = "$vsDir\package.json"
    if (Test-Path $pkgJson) {
        Push-Location $vsDir
        if (-not (Test-Path "node_modules")) {
            Write-Step "安装 Node.js 依赖..."
            npm install
        }
        $env:MEMOCHAT_HEALTH_PORT = "8082"
        Start-Process -FilePath "node" -ArgumentList "server.js" -WindowStyle Hidden
        Pop-Location
        Write-Success "VarifyServer 启动"
    }
}

# 等待服务就绪
Write-Step "等待服务就绪 (10秒)..."
Start-Sleep -Seconds 10

# ============================================================
# 步骤 4: 启动 QML 客户端 (可选)
# ============================================================
if (-not $SkipClient) {
    Write-Step "启动 QML 客户端..."

    $clientDir = "$ProjectRoot\client\MemoChat-qml"
    $clientExe = "$ServicesDir\GateServerHttp2\MemoChatQml.exe"

    if (Test-Path $clientExe) {
        Start-Process -FilePath $clientExe
        Write-Success "QML 客户端启动"
    } else {
        Write-Warn "QML 客户端未找到: $clientExe"
        Write-Host "手动启动: cd $clientDir && cmake --build . --config Release" -ForegroundColor Gray
    }
}

# ============================================================
# 最终状态
# ============================================================
Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                    启动完成                                  ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

Write-Host "服务端口:" -ForegroundColor Yellow
$ports = @(
    "GateServer HTTP : 8080",
    "GateServer gRPC : 50054",
    "StatusServer    : 50052",
    "VarifyServer    : 50051 / 8082",
    "ChatServer TCP  : 8090",
    "ChatServer QUIC : 8190",
    "PostgreSQL      : 5432",
    "Redis           : 6379",
    "MongoDB         : 27017",
    "Kafka           : 9092",
    "RabbitMQ AMQP   : 5672",
    "RabbitMQ Mgmt   : 15672"
)
$ports | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }

Write-Host @"

查看日志:
  Get-Content $ProjectRoot\Memo_ops\artifacts\runtime\logs\*.log -Tail 50 -Wait

停止服务:
  .\scripts\windows\stop_test_services.bat
"@ -ForegroundColor Cyan
