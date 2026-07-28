# scripts/ 目录树

> CI/CD 工作流调用的部署脚本。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `deploy-k8s.sh` | 仅部署旧 Kustomize 开发/预发布环境；生产路径 fail-closed 并要求使用受发布门禁保护的 Helm chart。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
