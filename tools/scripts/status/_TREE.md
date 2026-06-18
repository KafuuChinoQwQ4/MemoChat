# status/ 目录树

> 服务编排与运行时状态脚本集合：本地/全栈/k8s 启停、各 gRPC 微服务运行时冒烟、Phase2 拆库迁移与拓扑查看。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `cleanup-wsl-stale.ps1` | 清理 WSL 中的残留进程/资源 |
| `deploy_services.bat` | 部署服务的批处理脚本 |
| `deploy_services.ps1` | 部署服务的 PowerShell 脚本 |
| `deploy_services.sh` | 部署服务的 shell 脚本 |
| `ensure_minio_buckets.sh` | 确保 MinIO 所需 bucket 存在 |
| `migrate_phase2_account_split.sh` | Phase2 账号库拆分迁移 |
| `migrate_phase2_db_split.sh` | Phase2 数据库拆分迁移 |
| `run-linux-full-stack.sh` | 启动 Linux 全栈服务 |
| `run-service-console.ps1` | 以控制台方式运行单个服务 |
| `runtime_topology.sh` | 打印运行时服务拓扑 |
| `smoke_aigateway_runtime.sh` | AI Gateway 运行时冒烟 |
| `smoke_chatserver_default_grpc_runtime.sh` | ChatServer 默认 gRPC 运行时冒烟 |
| `smoke_domain_gateway_runtime.sh` | Domain Gateway 运行时冒烟 |
| `smoke_envoy_cutover_runtime.sh` | Envoy 切流运行时冒烟 |
| `smoke_message_service_grpc_runtime.sh` | Message 服务 gRPC 运行时冒烟 |
| `smoke_relation_query_grpc_runtime.sh` | Relation 查询服务 gRPC 运行时冒烟 |
| `smoke_relation_service_grpc_runtime.sh` | Relation 服务 gRPC 运行时冒烟 |
| `start-all-services.bat` | 启动全部服务（批处理） |
| `start-all-services.sh` | 启动全部服务（shell） |
| `start-local-k8s.ps1` | 启动本地 k8s 集群 |
| `start-memochat-qml-wslg.sh` | 在 WSLg 下启动 MemoChat QML 客户端 |
| `status-local-k8s.ps1` | 查看本地 k8s 状态 |
| `stop-all-services.bat` | 停止全部服务（批处理） |
| `stop-all-services.sh` | 停止全部服务（shell） |
| `stop-local-k8s.ps1` | 停止本地 k8s 集群 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
