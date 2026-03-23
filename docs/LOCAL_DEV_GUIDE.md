# MemoChat 本地开发环境启动指南

## 目录

- [前置条件](#前置条件)
- [方案一：Windows 原生服务 (推荐)](#方案一windows-原生服务-推荐)
- [方案二：Docker Desktop K8s](#方案二docker-desktop-k8s)
- [启动后验证](#启动后验证)
- [常见问题](#常见问题)

---

## 前置条件

### 1. 安装必要软件

| 软件 | 版本要求 | 下载地址 |
|------|----------|----------|
| Docker Desktop | 4.x+ | https://www.docker.com/products/docker-desktop |
| kubectl | 1.27+ | `winget install Microsoft.Kubectl` |
| Git | 2.x+ | https://git-scm.com/download/win |

### 2. 克隆项目

```powershell
git clone https://github.com/your-repo/MemoChat-Qml-Drogon.git D:\MemoChat-Qml-Drogon
cd D:\MemoChat-Qml-Drogon
```

### 3. 检查环境

```powershell
# 验证依赖
docker --version      # Docker Desktop
kubectl version       # kubectl
git --version         # Git
```

---

## 方案一：Windows 原生服务 (推荐)

适合 **本地开发调试**，服务直接在 Windows 上运行，启动快。

### 启动步骤

```powershell
cd D:\MemoChat-Qml-Drogon

# 1. 启动基础设施 (PostgreSQL, Redis, MongoDB, Kafka, RabbitMQ)
.\scripts\windows\start_test_services.bat

# 2. 等待服务就绪 (约 30 秒)
Start-Sleep 30

# 3. 构建 C++ 服务器 (如果需要)
cd build
cmake --build . --config Release
cd ..

# 4. 启动 GateServer
.\Memo_ops\bin\GateServer.exe --config Memo_ops\config\GateServer.json

# 5. 启动 ChatServer (多个实例)
.\Memo_ops\bin\ChatServer.exe --config Memo_ops\config\ChatServer1.json
.\Memo_ops\bin\ChatServer.exe --config Memo_ops\config\ChatServer2.json
.\Memo_ops\bin\ChatServer.exe --config Memo_ops\config\ChatServer3.json
.\Memo_ops\bin\ChatServer.exe --config Memo_ops\config\ChatServer4.json

# 6. 启动 StatusServer
.\Memo_ops\bin\StatusServer.exe --config Memo_ops\config\StatusServer.json

# 7. 启动 VarifyServer (Node.js)
cd Memo_ops\services\VarifyServer
npm install
node src\index.js

# 8. 启动 QML 客户端
cd client\MemoChat-qml
qmake MemoChat.pro
cmake --build . --config Release
Release\MemoChatQml.exe
```

### 停止服务

```powershell
.\scripts\windows\stop_test_services.bat
```

---

## 方案二：Docker Desktop K8s

适合 **生产环境模拟**，所有服务跑在 Kubernetes 集群里。

### 启动步骤

#### 第一步：启用 Docker Desktop Kubernetes

1. 打开 **Docker Desktop** → Settings → Kubernetes
2. 勾选 **Enable Kubernetes**
3. 点击 **Apply & Restart**
4. 等待启动完成（约 2-5 分钟）

#### 第二步：验证 K8s 集群

```powershell
kubectl cluster-info
kubectl get nodes
```

#### 第三步：启动服务

```powershell
cd D:\MemoChat-Qml-Drogon

# 启动 K8s 部署 (自动部署 etcd + 所有服务)
.\scripts\start-local-k8s.ps1

# 或仅启动最小化版本 (etcd + 基础服务)
.\scripts\start-local-k8s.ps1 -Mode minimal
```

#### 第四步：查看状态

```powershell
.\scripts\status-local-k8s.ps1
```

#### 停止服务

```powershell
# 仅删除 K8s 资源
.\scripts\stop-local-k8s.ps1

# 完全清理 (包括 PVC)
.\scripts\stop-local-k8s.ps1 -CleanAll
```

---

## 启动后验证

### 1. 检查服务端口

```powershell
# Windows 原生方案
netstat -ano | findstr "8080 8090 50054 5432 6379 27017"
```

```powershell
# K8s 方案
.\scripts\status-local-k8s.ps1
```

### 2. 健康检查

```powershell
# GateServer 健康检查
curl http://localhost:8080/health

# ChatServer 健康检查
curl http://localhost:8090/health
```

### 3. 测试登录

```powershell
# 使用测试脚本
python .\scripts\test_login.py --host localhost --port 8080 --user testuser --password test123
```

---

## 服务端口映射

| 服务 | 端口 | 说明 |
|------|------|------|
| GateServer HTTP | 8080 | REST API 入口 |
| GateServer gRPC | 50054 | RPC 端口 |
| StatusServer | 50052 | 状态服务 RPC |
| VarifyServer | 50051 / 8081 | 验证码服务 |
| ChatServer TCP | 8090 | 聊天 TCP |
| ChatServer QUIC | 8190 | 聊天 QUIC |
| PostgreSQL | 5432 | 数据库 |
| Redis | 6379 | 缓存 |
| MongoDB | 27017 | 文档存储 |
| Kafka | 9092 / 19092 | 消息队列 |
| RabbitMQ | 5672 / 15672 | 异步任务 |
| etcd | 2379 | 配置中心 |

---

## 常见问题

### Q1: Docker Desktop Kubernetes 启用失败

**症状**: 点击 Apply 后报错 "Kubernetes failed to start"

**解决**:
1. 检查 WSL2 或 Hyper-V 是否启用
2. 重置 Docker Desktop: Settings → Troubleshoot → Reset to factory defaults
3. 查看日志: Docker Desktop → Troubleshoot → View logs

### Q2: PVC 创建失败

**症状**: `Pending` 状态的 PVC

**解决**:
```powershell
kubectl describe pvc <pvc-name> -n memochat
# 检查 StorageClass 是否存在
kubectl get storageclass
```

### Q3: 服务 Pod 无法启动

**症状**: `CrashLoopBackOff` 或 `ImagePullBackOff`

**解决**:
```powershell
# 查看日志
kubectl logs <pod-name> -n memochat

# 如果是 ImagePullBackOff，需要先构建并推送镜像
docker build -t memochat/gateserver:latest -f docker/Dockerfile.server .
docker push memochat/gateserver:latest
```

### Q4: 端口冲突

**症状**: `Bind error: Address already in use`

**解决**:
```powershell
# 查找占用端口的进程
netstat -ano | findstr "<port>"

# 终止进程
taskkill /PID <pid> /F
```

### Q5: Windows 原生方案连接数据库失败

**症状**: `Connection refused to localhost:5432`

**解决**:
1. 确保 PostgreSQL 容器正在运行:
   ```powershell
   docker ps | findstr postgres
   ```
2. 检查端口映射:
   ```powershell
   docker port <container-id>
   ```

---

## 环境变量参考

### GateServer

```bash
MEMOCHAT_POSTGRES_HOST=localhost
MEMOCHAT_REDIS_HOST=localhost
MEMOCHAT_MONGODB_URI=mongodb://localhost:27017
MEMOCHAT_KAFKA_BROKERS=localhost:19092
MEMOCHAT_RABBITMQ_HOST=localhost
MEMOCHAT_LOG_LEVEL=debug
```

### ChatServer

```bash
MEMOCHAT_CLUSTER_DISCOVERYMODE=etcd          # 或 direct
MEMOCHAT_CLUSTER_ETCDENDPOINTS=http://localhost:2379
MEMOCHAT_POSTGRES_HOST=localhost
MEMOCHAT_REDIS_HOST=localhost
```

---

## 快速命令速查

```powershell
# ===== Windows 原生方案 =====
.\scripts\windows\start_test_services.bat    # 启动基础设施
.\scripts\windows\stop_test_services.bat     # 停止基础设施

# ===== K8s 方案 =====
.\scripts\start-local-k8s.ps1               # 启动 K8s 部署
.\scripts\stop-local-k8s.ps1                # 停止 K8s 部署
.\scripts\status-local-k8s.ps1             # 查看 K8s 状态

# ===== 通用 =====
kubectl get pods -n memochat                # 查看 Pods
kubectl logs -f <pod> -n memochat            # 查看日志
kubectl port-forward svc/memochat-gateserver 8080:8080 -n memochat  # 端口转发
```
