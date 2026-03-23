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
    .\start-local-k8s.ps1 -RebuildImage    # 先重新构建镜像再部署（本地开发用）
#>

param(
    [string]$Mode = "full",
    [switch]$SkipEtcd,
    [switch]$RebuildImage
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
# 步骤 1.5: 重新构建镜像（可选）
# ============================================================
if ($RebuildImage) {
    Write-Step "重新构建 Docker 镜像..."

    $buildDir = "$ProjectRoot\build\bin\Release"
    $runtimeDir = "$ProjectRoot\Memo_ops\runtime\services"
    # 镜像名需与 K8s overlay (Memo_ops/k8s/overlays/dev/kustomization.yaml) 中 images 字段一致
    $imageNameMap = @{
        "GateServer"   = "memochat/gateserver"
        "ChatServer"   = "memochat/chatserver"
        "StatusServer" = "memochat/statusserver"
        "VarifyServer" = "memochat/varifyserver"
    }
    $imageTag = "dev-latest"

    # 服务列表
    $services = @("GateServer", "ChatServer", "StatusServer", "VarifyServer")

    foreach ($svc in $services) {
        $srcDir = "$buildDir"
        $destDir = "$runtimeDir\$svc"
        $imageFull = "$($imageNameMap[$svc]):$imageTag"

        # 同步编译产物到 runtime 目录
        Write-Host "  同步 $svc 编译产物..." -ForegroundColor Gray
        if (-not (Test-Path $srcDir)) {
            Write-Warn "  跳过 $svc：编译产物目录不存在 ($srcDir)"
            continue
        }

        $exeFiles = Get-ChildItem -Path $srcDir -Filter "*.exe" -File | Where-Object { $_.Name -like "*$svc*" }
        if (-not $exeFiles) {
            Write-Warn "  跳过 $svc：未找到 $svc.exe"
            continue
        }

        # 创建目标目录并复制
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        Copy-Item -Path "$srcDir\*" -Destination $destDir -Recurse -Force -Exclude @("*.pdb", "CMakeFiles", "CMakeCache.txt")

        # 构建 Docker 镜像
        Write-Host "  构建 $imageFull ..." -ForegroundColor Gray
        docker build `
            -f "$ProjectRoot\docker\Dockerfile.server" `
            --build-arg SERVICE=$svc `
            --build-arg SOURCE_DIR=Memo_ops/runtime/services/$svc `
            -t $imageFull `
            --load `
            $ProjectRoot 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }

        if ($LASTEXITCODE -ne 0) {
            Write-Fail "$svc 镜像构建失败"
            exit 1
        }
        Write-Success "  $svc 镜像 ($imageFull) 构建完成"
    }

    Write-Success "所有镜像构建完毕，开始部署..."
} else {
    Write-Host "[跳过] 使用已有镜像（添加 -RebuildImage 强制重新构建）" -ForegroundColor Gray
}

# ============================================================
# 步骤 2: 启动本地 etcd (Docker 方式)
# ============================================================
if (-not $SkipEtcd) {
    Write-Step "启动本地 etcd..."

    # 检查是否已有 etcd 容器运行
    $etcdContainer = docker ps -a --filter "name=memochat-etcd" --format "{{.Names}}"
    if ($etcdContainer) {
        Write-Warn "已有 etcd 容器: $etcdContainer"
        docker start memochat-etcd 2>&1 | Out-Null
    } else {
        # 启动新的 etcd 容器
        docker run -d `
            --name memochat-etcd `
            -p 2379:2379 `
            -p 2380:2380 `
            --restart unless-stopped `
            -e ALLOW_NONE_AUTHENTICATION=yes `
            -e ETCD_ADVERTISE_CLIENT_URLS=http://localhost:2379 `
            -e ETCD_INITIAL_CLUSTER_STATE=new `
            -e ETCD_LISTEN_CLIENT_URLS=http://0.0.0.0:2379 `
            bitnami/etcd:3.5.9 2>&1 | Out-Null

        if ($LASTEXITCODE -eq 0) {
            Write-Success "etcd 容器已启动"
        } else {
            Write-Fail "etcd 启动失败"
            exit 1
        }
    }

    # 等待 etcd 就绪
    Write-Step "等待 etcd 就绪..."
    $retry = 0
    while ($retry -lt 30) {
        $result = docker exec memochat-etcd etcdctl endpoint health 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Success "etcd 已就绪"
            break
        }
        Start-Sleep -Seconds 1
        $retry++
    }
    if ($retry -ge 30) {
        Write-Fail "etcd 启动超时"
        exit 1
    }
} else {
    Write-Warn "跳过 etcd 启动"
}

# ============================================================
# 步骤 3: 等待 Kubernetes 就绪
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
