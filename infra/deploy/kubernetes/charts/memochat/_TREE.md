# memochat/ 目录树

> MemoChat 全栈 Helm chart：模板、多环境 values 与生产可选 profiles，按需部署业务服务、负载均衡与可观测性。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`templates/`](templates/_TREE.md) | Helm 模板（bootstrap、lb、observability、ops、prod、shared）。 |
| [`values/`](values/_TREE.md) | 多环境 values 覆盖文件。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `.helmignore` | 排除模板目录中的仓库导航文档，避免 Helm 将其渲染为 Kubernetes 清单。 |
| `Chart.yaml` | Helm chart 元信息。 |
| `README.md` | chart 使用说明。 |
| `values.yaml` | 默认 values、GHCR 镜像清单、角色凭据和可选服务开关配置。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
