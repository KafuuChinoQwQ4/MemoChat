# prod/ 目录树

> MemoOps k8s 生产环境 kustomize overlay，启用服务网格 mTLS、外部密钥同步并删除 base 占位 Secret。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `external-secrets.yaml` | 生产环境 External Secrets Operator/Vault SecretStore 与 ExternalSecret 映射。 |
| `kustomization.yaml` | 生产环境 kustomize 配置，设置副本/镜像、服务网格标签并删除 base 占位 Secret。 |
| `mesh-istio.yaml` | 生产环境 Istio strict mTLS、DestinationRule 与 namespace AuthorizationPolicy。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
