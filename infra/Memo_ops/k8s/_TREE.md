# k8s/ 目录树

> MemoOps 及被监控后端/基础设施的 Kubernetes 清单，按 base/overlay 分层，含多环境 kustomize overlay。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`backend/`](backend/_TREE.md) | 后端业务服务（chat/gate/varify）的 k8s 清单。 |
| [`base/`](base/_TREE.md) | 命名空间、ConfigMap、Secret 等基础资源。 |
| [`etcd/`](etcd/_TREE.md) | etcd StatefulSet 清单。 |
| [`infra/`](infra/_TREE.md) | 中间件（kafka/mongo/postgres/rabbitmq/redis）清单。 |
| [`overlays/`](overlays/_TREE.md) | 多环境 kustomize overlay（dev/staging/prod 等）。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
