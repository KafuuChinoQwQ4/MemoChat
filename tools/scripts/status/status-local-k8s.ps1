<#
.SYNOPSIS
    查看 MemoChat 本地 K8s 状态
.EXAMPLE
    .\status-local-k8s.ps1
#>

$ErrorActionPreference = "Continue"

# 颜色输出函数
function Write-Section { param($msg) Write-Host "`n=== $msg ===" -ForegroundColor Yellow }

Write-Host @"
╔═══════════════════════════════════════════════════════════════╗
║              MemoChat 本地 K8s 状态                           ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

# ============================================================
# Docker 状态
# ============================================================
Write-Section "Docker 容器"
docker ps -a --filter "name=memochat" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

# ============================================================
# Kubernetes 状态
# ============================================================
Write-Section "Kubernetes Pods"
kubectl get pods -n memochat -o wide 2>&1
kubectl get pods -n memochat-dev -o wide 2>&1

Write-Section "Kubernetes Services"
kubectl get svc -n memochat 2>&1
kubectl get svc -n memochat-dev 2>&1

Write-Section "Kubernetes Deployments"
kubectl get deploy -n memochat 2>&1
kubectl get deploy -n memochat-dev 2>&1

Write-Section "PVCs"
kubectl get pvc -A 2>&1 | Where-Object { $_ -match "memochat" }

# ============================================================
# 端口映射
# ============================================================
Write-Section "端口映射"

$ports = @(
    @{ Name = "GateServer HTTP"; Port = "8080" },
    @{ Name = "GateServer gRPC"; Port = "50054" },
    @{ Name = "StatusServer"; Port = "50052" },
    @{ Name = "VarifyServer gRPC"; Port = "50051" },
    @{ Name = "VarifyServer HTTP"; Port = "8081" },
    @{ Name = "ChatServer TCP"; Port = "8090" },
    @{ Name = "ChatServer QUIC"; Port = "8190" },
    @{ Name = "etcd"; Port = "2379" },
    @{ Name = "PostgreSQL"; Port = "5432" },
    @{ Name = "Redis"; Port = "6379" },
    @{ Name = "MongoDB"; Port = "27017" },
    @{ Name = "Kafka"; Port = "9092" },
    @{ Name = "Kafka External"; Port = "19092" },
    @{ Name = "RabbitMQ AMQP"; Port = "5672" },
    @{ Name = "RabbitMQ Mgmt"; Port = "15672" }
)

Write-Host "`n服务端口:" -ForegroundColor Cyan
$ports | ForEach-Object {
    $status = if (Test-NetConnection -ComputerName localhost -Port $_.Port -WarningAction SilentlyContinue -InformationLevel Quiet) { "✓ 监听中" } else { "✗ 未监听" }
    $color = if ($status -eq "✓ 监听中") { "Green" } else { "Gray" }
    Write-Host ("  {0,-20} : {1,-12} {2}" -f $_.Name, $_.Port, $status) -ForegroundColor $color
}

Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                    常用命令                                  ║
╚═══════════════════════════════════════════════════════════════╝

启动: .\scripts\start-local-k8s.ps1
停止: .\scripts\stop-local-k8s.ps1

查看日志:
  kubectl logs -n memochat -l app=memochat-gateserver -f
  kubectl logs -n memochat -l app=memochat-chatserver -f

进入容器:
  kubectl exec -it <pod-name> -n memochat -- /bin/sh

端口转发:
  kubectl port-forward -n memochat svc/memochat-gateserver 8080:8080
"@ -ForegroundColor Magenta
