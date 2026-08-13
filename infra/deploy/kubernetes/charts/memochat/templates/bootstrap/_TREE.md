# bootstrap/ 目录树

> Helm 初始化资源模板：命名空间、ConfigMap、Secret 与一次性 Job。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `configmap-ops-init.yaml` | 平台初始化 ConfigMap，内含消息设施脚本、账号/R18 迁移与基础 SQL；OTel 配置按 observability profile 渲染。 |
| `configmap-services.yaml` | 各服务通用配置 ConfigMap 模板，包含 relation command/query 服务及其账号库和消息总线配置链。 |
| `external-secrets.yaml` | External Secrets Operator/Vault SecretStore 与 ExternalSecret 模板。 |
| `jobs.yaml` | 初始化/迁移类 Job 模板，先失败关闭地迁移 `memo_account` 再执行其他平台初始化。 |
| `mesh-istio.yaml` | Istio strict mTLS 与 namespace AuthorizationPolicy 模板。 |
| `namespaces.yaml` | 生产、运维命名空间模板，以及按 observability 开关创建的观测命名空间。 |
| `secret.yaml` | 生产镜像门禁、四类 relation token 与 Postgres 角色凭据 Secret 模板。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
