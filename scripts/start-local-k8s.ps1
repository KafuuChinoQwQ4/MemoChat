<#
.SYNOPSIS
    MemoChat 本地 K8s 一键启动脚本
.DESCRIPTION
    在 Windows 本地启动 etcd + K8s + MemoChat 服务集群
    依赖: Docker Desktop (已启用 Kubernetes)
.PARAMETER Mode
    部署模式: full (完整) 或 minimal (最小化，仅 etcd + 基础服务)
.PARAMETER SkipEtcd
    跳过 etcd 启动（如果已手动启动）
.EXAMPLE
    .\start-local-k8s.ps1
    .\start-local-k8s.ps1 -Mode minimal
    .\start-local-k8s.ps1 -SkipEtcd
#>

param(
    [string]$Mode = "full",
    [switch]$SkipEtcd
)

$ErrorActionPreference = "Stop"
$ProjectRoot = "D:\MemoChat-Qml-Drogon"
$K8sDir = "$ProjectRoot\Memo_ops\k8s"
$Overlay = "dev-single"

# 颜色输出函数
function Write-Step { param($msg) Write-Host "[步骤] $msg" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[成功] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[警告] $msg" -ForegroundColor Yellow }
function Write-Fail { param($msg) Write-Host "[错误] $msg" -ForegroundColor Red }

Write-Host @"
╔═══════════════════════════════════════════════════════════════╗
║         MemoChat 本地 K8s 开发环境启动器                        ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

# ============================================================
# 步骤 1: 检查 Docker Desktop
# ============================================================
Write-Step "检查 Docker Desktop 状态..."

$dockerInfo = docker info 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Fail "Docker 未运行，请先启动 Docker Desktop"
    exit 1
}

if ($dockerInfo -match "Kubernetes.*Running") {
    Write-Success "Docker Kubernetes 已启用"
    $k8sEnabled = $true
} else {
    Write-Warn "Docker Kubernetes 未启用"
    Write-Host "请在 Docker Desktop 设置中启用 Kubernetes，然后重启 Docker" -ForegroundColor Yellow
    $k8sEnabled = $false
}

# ============================================================
# 步骤 2: 等待 Kubernetes 就绪
# ============================================================
if ($k8sEnabled) {
    Write-Step "检查 Kubernetes 集群..."
    kubectl cluster-info 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "Kubernetes 集群未就绪"
        exit 1
    }
    Write-Success "Kubernetes 集群正常"

    # ============================================================
    # 步骤 4: 部署 MemoChat 到 K8s
    # ============================================================
    Write-Step "部署 MemoChat ($Mode 模式)..."

    # 创建命名空间
    kubectl apply -f "$K8sDir\base\namespace.yaml" 2>&1 | Out-Null

    # 部署配置和密钥
    kubectl apply -f "$K8sDir\base\configmap.yaml" 2>&1 | Out-Null
    kubectl apply -f "$K8sDir\base\secrets.yaml" 2>&1 | Out-Null

    # 根据模式选择部署
    if ($Mode -eq "minimal") {
        # 仅部署 etcd + 基础服务
        Write-Host "  -> minimal 模式: 仅部署 etcd" -ForegroundColor Gray

        # 部署单节点 etcd
        kubectl apply -f "$K8sDir\etcd\statefulset.yaml" 2>&1 | Out-Null
    } else {
        # 完整部署
        Write-Host "  -> full 模式: 部署所有服务" -ForegroundColor Gray

        # 部署基础设施 (postgres, redis, mongodb, kafka, rabbitmq)
        kubectl apply -k "$K8sDir\infra\" 2>&1 | Out-Null

        # 部署后端服务
        kubectl apply -k "$K8sDir\backend\" 2>&1 | Out-Null
    }

    # 等待部署完成
    Write-Step "等待服务启动..."
    Start-Sleep -Seconds 5

    # ============================================================
    # 步骤 5: 显示状态
    # ============================================================
    Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                    服务状态                                    ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

    Write-Host "`n=== Pods ===" -ForegroundColor Yellow
    kubectl get pods -n memochat -o wide 2>&1

    Write-Host "`n=== Services ===" -ForegroundColor Yellow
    kubectl get svc -n memochat 2>&1

    Write-Host "`n=== 访问地址 ===" -ForegroundColor Yellow
    Write-Host "  GateServer HTTP:  http://localhost:8080" -ForegroundColor Green
    Write-Host "  GateServer gRPC:  localhost:50054" -ForegroundColor Green
    Write-Host "  StatusServer:     localhost:50052" -ForegroundColor Green
    Write-Host "  VarifyServer:    localhost:50051 (gRPC), localhost:8081 (HTTP)" -ForegroundColor Green
    Write-Host "  etcd:            localhost:2379" -ForegroundColor Green
    Write-Host "  PostgreSQL:      localhost:5432" -ForegroundColor Green
    Write-Host "  Redis:           localhost:6379" -ForegroundColor Green
    Write-Host "  MongoDB:         localhost:27017" -ForegroundColor Green
    Write-Host "  Kafka:           localhost:9092" -ForegroundColor Green
    Write-Host "  RabbitMQ:        localhost:5672 (AMQP), localhost:15672 (Management)" -ForegroundColor Green

    Write-Host "`n=== 查看日志 ===" -ForegroundColor Yellow
    Write-Host "  kubectl logs -n memochat -l app=memochat-gateserver" -ForegroundColor Gray
    Write-Host "  kubectl logs -n memochat -l app=memochat-chatserver" -ForegroundColor Gray

    Write-Host "`n=== 停止服务 ===" -ForegroundColor Yellow
    Write-Host "  kubectl delete -k $K8sDir\overlays\$Overlay" -ForegroundColor Gray
} else {
    Write-Warn "Kubernetes 未启用，仅启动 etcd"
    Write-Host "`n手动启动 K8s 后运行:" -ForegroundColor Yellow
    Write-Host "  kubectl apply -k $K8sDir\overlays\$Overlay" -ForegroundColor Gray
}

Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                    启动完成                                    ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta
