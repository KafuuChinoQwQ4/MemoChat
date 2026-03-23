<#
.SYNOPSIS
    MemoChat 本地 K8s 停止脚本
.DESCRIPTION
    停止并清理本地 K8s 部署
.PARAMETER CleanAll
    清理所有资源（包括 PVC 和 etcd 容器）
.EXAMPLE
    .\stop-local-k8s.ps1
    .\stop-local-k8s.ps1 -CleanAll
#>

param(
    [switch]$CleanAll
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
║         MemoChat 本地 K8s 停止与清理                          ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

# ============================================================
# 步骤 1: 删除 K8s 部署
# ============================================================
Write-Step "删除 K8s 部署..."

if (Test-Path "$K8sDir\overlays\$Overlay\kustomization.yaml") {
    kubectl delete -k "$K8sDir\overlays\$Overlay" --ignore-not-found 2>&1 | Out-Null
}

# 删除命名空间级资源
kubectl delete namespace memochat --ignore-not-found 2>&1 | Out-Null
kubectl delete namespace memochat-dev --ignore-not-found 2>&1 | Out-Null

Write-Success "K8s 资源已删除"

# ============================================================
# 步骤 2: 清理 PVC (可选)
# ============================================================
if ($CleanAll) {
    Write-Step "清理 PVC..."
    kubectl delete pvc --all -n memochat --ignore-not-found 2>&1 | Out-Null
    kubectl delete pvc --all -n memochat-dev --ignore-not-found 2>&1 | Out-Null
    Write-Success "PVC 已清理"
}

# ============================================================
# 步骤 3: 停止 etcd 容器
# ============================================================
if ($CleanAll) {
    Write-Step "停止并删除 etcd 容器..."
    docker stop memochat-etcd 2>&1 | Out-Null
    docker rm memochat-etcd 2>&1 | Out-Null
    Write-Success "etcd 容器已删除"
} else {
    Write-Step "停止 etcd 容器..."
    docker stop memochat-etcd 2>&1 | Out-Null
    Write-Success "etcd 容器已停止"
}

# ============================================================
# 步骤 4: 显示最终状态
# ============================================================
Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                    清理完成                                  ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

Write-Host "当前状态:" -ForegroundColor Yellow
Write-Host "  K8s Pods:" -ForegroundColor Gray
kubectl get pods -A 2>&1 | Where-Object { $_ -match "memochat" } | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }

Write-Host "  Docker 容器:" -ForegroundColor Gray
docker ps -a --filter "name=memochat" --format "{{.Names}}: {{.Status}}" 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }

Write-Host @"

如需重新启动:
  .\scripts\start-local-k8s.ps1
"@ -ForegroundColor Green
