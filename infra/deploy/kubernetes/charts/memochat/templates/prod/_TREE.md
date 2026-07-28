# prod/ 目录树

> Helm 业务服务模板：AI、聊天、网关、验证等服务部署与自动扩缩容（HPA）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ai.yaml` | AI 编排服务部署模板。 |
| `chat.yaml` | ChatServer 与内部 relation command/query gRPC 服务的工作负载、Service 和健康探针模板。 |
| `focused-gateways.yaml` | 可独立启停的专用网关部署模板，按域注入 relation token 与 Postgres 角色凭据。 |
| `gate.yaml` | 仅在 `legacyGate.enabled` 时渲染的旧 GateServer 部署模板。 |
| `hpa.yaml` | 仅为已启用业务工作负载渲染的水平自动扩缩容模板。 |
| `varify.yaml` | VarifyServer（验证服务）部署模板。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
