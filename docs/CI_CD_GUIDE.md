# MemoChat CI/CD 自动部署指南

> 当前版本：2026-04-26
> 当前目录基准：镜像和部署配置在 `infra/deploy`，Kubernetes Chart 在 `infra/deploy/kubernetes` 和 `infra/helm`。本地开发依赖 Docker-only；生产环境应使用 Secret/ConfigMap 覆盖本地固定密码和端口。

## 概述

MemoChat 使用 GitHub Actions 实现 CI/CD，代码 push 后自动构建 Docker 镜像并部署到 Kubernetes 集群。

当前仓库已经拆分为 `apps/`、`infra/`、`tools/` 三个主要域。CI 需要分别覆盖：

- C++ 服务端：`apps/server/core`
- Qt/QML 客户端：`apps/client/desktop`
- 运营平台：`infra/Memo_ops`
- 压测工具：`tools/loadtest/python-loadtest`
- 部署资产：`infra/deploy`
- 测试：`tests`

本地 Docker-only 约束不直接等同于生产部署。生产环境可以使用 Kubernetes 内部 PostgreSQL `5432` 或托管数据库，但本地开发固定使用 Docker 暴露的 `15432`。CI/CD 中不能硬编码本地密码和本地端口，应通过环境变量、Secret 或 Helm values 注入。

## 当前构建边界

| 目标 | 推荐检查 |
|------|----------|
| C++ 编译 | `cmake --preset ...` + `cmake --build ...` |
| 单元测试 | `ctest` 或直接运行 GTest 可执行 |
| Docker 镜像 | `infra/deploy/images` 下的 Dockerfile |
| Helm/K8s | `infra/deploy/kubernetes`、`infra/helm` |
| 文档 | `docs` 中路径和端口不得回退到旧结构 |

建议 CI 至少做以下静态检查：

```powershell
rg -n "localhost:5432|Port=5432|memochat-mysql|D:\\\\MemoChat\\\\server" docs apps infra tools
```

允许 Kubernetes 内部端口 `5432` 出现在 chart values 中，但必须清楚标注它是集群内部端口。

## 部署流程

```
┌─────────────────────────────────────────────────────────────────┐
│                        代码 Push                                │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    GitHub Actions                               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│  │   CI        │───▶│   CD        │───▶│  部署完成     │        │
│  │ 构建+测试    │    │  推送镜像    │    │  K8s 更新    │        │
│  │             │    │  部署       │    │             │        │
│  └─────────────┘    └─────────────┘    └─────────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

## 分支策略

| 分支 | 触发 CI | 触发 CD | 部署环境 | 副本数 |
|------|---------|---------|----------|--------|
| `develop` | ✅ | ✅ | Dev | 1-2 |
| `main` | ✅ | ✅ | Production | 3-4 |
| `feature/*` | ✅ | ⚠️ 可选 | Staging | 2-3 |

## GitHub Secrets 配置

CD 部署到远程 K8s 集群需要配置以下 Secrets：

### 1. DEV_KUBECONFIG

开发环境的 kubeconfig 文件（base64 编码）

```powershell
# 在本地生成
kubectl config view --raw > kubeconfig
# 或从云服务商获取 kubeconfig 文件

# Base64 编码
[Convert]::ToBase64String([System.IO.File]::ReadAllBytes("kubeconfig"))

# 在 GitHub 仓库 Settings → Secrets → Actions 中添加
# Name: DEV_KUBECONFIG
# Value: <base64 编码内容>
```

### 2. PROD_KUBECONFIG

生产环境的 kubeconfig 文件（base64 编码）

**注意：生产环境配置需要额外谨慎！**

```powershell
# 同样方式生成并编码
[Convert]::ToBase64String([System.IO.File]::ReadAllBytes("kubeconfig"))

# 在 GitHub 仓库 Settings → Secrets → Actions 中添加
# Name: PROD_KUBECONFIG
# Value: <base64 编码内容>
```

## GitHub Actions 环境配置

### 1. 创建设境

1. 进入 GitHub 仓库 → **Settings** → **Environments**
2. 点击 **New environment**
3. 创建 `dev` 和 `production` 环境
4. 为生产环境配置 **required reviewers**（可选但推荐）

### 2. 配置环境变量

| 环境 | 变量名 | 示例值 |
|------|--------|--------|
| dev | `KUBECONFIG` | 已通过 Secret 注入 |
| production | `KUBECONFIG` | 已通过 Secret 注入 |

## 本地测试 CD 配置

### 1. 使用 Docker Desktop 本地 K8s 测试

```powershell
# 获取本地 K8s kubeconfig
kubectl config view --raw > local-kubeconfig.yaml

# Base64 编码
$bytes = [System.IO.File]::ReadAllBytes("local-kubeconfig.yaml")
[Convert]::ToBase64String($bytes)

# 临时测试：修改 cd.yml 改用本地配置
#   echo "${{ secrets.LOCAL_KUBECONFIG }}" | base64 -d > kubeconfig
```

### 2. 测试部署流程

```powershell
# 1. 手动触发 CI
git commit --allow-empty -m "test: trigger CI"
git push

# 2. 查看 Actions 日志
#   GitHub → Actions → 点击 workflow run → 查看日志

# 3. 手动触发 CD（如果 CI 通过）
#   GitHub → Actions → 点击 CI → Re-run jobs
```

## 镜像仓库

### GitHub Container Registry (ghcr.io)

| 服务 | 镜像地址 |
|------|----------|
| GateServer | `ghcr.io/<owner>/memochat/gateserver:<tag>` |
| StatusServer | `ghcr.io/<owner>/memochat/statusserver:<tag>` |
| ChatServer | `ghcr.io/<owner>/memochat/chatserver:<tag>` |
| VarifyServer | `ghcr.io/<owner>/memochat/varifyserver:<tag>` |

### 镜像标签策略

| 环境 | 标签 | 示例 |
|------|------|------|
| Dev | `dev-latest` | `ghcr.io/xxx/gateserver:dev-latest` |
| Staging | `staging-latest` | `ghcr.io/xxx/gateserver:staging-latest` |
| Production | `latest` + 时间戳 | `ghcr.io/xxx/gateserver:latest` |

## 查看部署状态

### GitHub Actions

```
GitHub 仓库 → Actions → All workflows → 选择 run 查看日志
```

### Kubernetes

```powershell
# 查看 pods
kubectl get pods -n memochat      # 生产
kubectl get pods -n memochat-dev  # 开发

# 查看服务
kubectl get svc -n memochat

# 查看部署状态
kubectl get deploy -n memochat

# 查看滚动更新历史
kubectl rollout history deployment/memochat-gateserver -n memochat
```

## 回滚

### 自动回滚（如果健康检查失败）

K8s 会自动阻止不健康的 Pod 升级，保留旧版本。

### 手动回滚

```powershell
# 回滚到上一个版本
kubectl rollout undo deployment/memochat-gateserver -n memochat

# 回滚到指定版本
kubectl rollout undo deployment/memochat-gateserver -n memochat --to-revision=3

# 验证回滚
kubectl rollout status deployment/memochat-gateserver -n memochat
```

## 生产环境注意事项

### 1. 审批流程

建议为生产环境配置 **required reviewers**：

1. GitHub → Settings → Environments → production
2. 勾选 **Required reviewers**
3. 添加至少 1-2 名审批者

### 2. 部署保护规则

```yaml
# cd.yml 中的配置
environment:
  name: production
  url: https://memochat.example.com
```

### 3. 部署窗口

建议在低峰期部署（如凌晨），或使用 **concurrency** 防止并发部署：

```yaml
concurrency:
  group: prod-deploy
  cancel-in-progress: false  # false = 排队等待，true = 取消正在进行的
```

## 常见问题

### Q1: 部署失败 "ImagePullBackOff"

**原因**: 镜像未推送到 registry 或权限不足

**解决**:
```powershell
# 检查镜像是否存在
docker images | grep memochat

# 手动推送
docker tag memochat/gateserver:latest ghcr.io/<owner>/memochat/gateserver:dev-latest
docker push ghcr.io/<owner>/memochat/gateserver:dev-latest
```

### Q2: 部署失败 " CrashLoopBackOff"

**原因**: 应用启动失败

**解决**:
```powershell
# 查看日志
kubectl logs <pod-name> -n memochat --previous

# 查看事件
kubectl describe pod <pod-name> -n memochat
```

### Q3: 部署卡住 "Waiting for rollout"

**原因**: 健康检查超时

**解决**:
```powershell
# 增加超时时间或检查服务状态
kubectl describe deployment <name> -n memochat

# 或手动回滚
kubectl rollout undo deployment/<name> -n memochat
```

## 更新日志

每次部署后，GitHub Actions 会：
1. 更新镜像标签
2. 执行 `kubectl apply`
3. 等待 RollingUpdate 完成
4. 记录部署版本到 annotation

```powershell
# 查看部署版本
kubectl annotate deployment/memochat-gateserver -n memochat --overwrite \
  kubernetes.io/change-cause="Deployed at 20260323-143000 from abc123"
```
