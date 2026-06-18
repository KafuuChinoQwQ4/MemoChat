# prod/ 目录树

> Helm 业务服务模板：AI、聊天、网关、验证等服务部署与自动扩缩容（HPA）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ai.yaml` | AI 编排服务部署模板。 |
| `chat.yaml` | ChatServer 部署模板。 |
| `focused-gateways.yaml` | 拆分后的专用网关（Auth/Media/Moments 等）部署模板。 |
| `gate.yaml` | GateServer 部署模板。 |
| `hpa.yaml` | 各服务水平自动扩缩容（HPA）模板。 |
| `varify.yaml` | VarifyServer（验证服务）部署模板。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
