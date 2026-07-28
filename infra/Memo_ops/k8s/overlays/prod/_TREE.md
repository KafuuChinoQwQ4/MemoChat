# prod/ 目录树

> 已退役且故意不可构建的 MemoOps k8s 生产环境 kustomize 哨兵；生产发布必须使用 `infra/deploy/kubernetes/charts/memochat`。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `kustomization.yaml` | 引用一个故意不存在的退役哨兵资源，使任何旧生产 kustomize 构建失败关闭。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
