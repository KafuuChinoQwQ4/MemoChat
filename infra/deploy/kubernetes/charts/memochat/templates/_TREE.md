# templates/ 目录树

> MemoChat Helm chart 模板根目录：按职责分组的 k8s 资源模板与共享 helper。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`bootstrap/`](bootstrap/_TREE.md) | 初始化资源：命名空间、ConfigMap、Secret、Job。 |
| [`lb/`](lb/_TREE.md) | Envoy 负载均衡/网关模板。 |
| [`observability/`](observability/_TREE.md) | 监控与 OTel 可观测性模板。 |
| [`ops/`](ops/_TREE.md) | MemoOps 运维平台部署模板。 |
| [`prod/`](prod/_TREE.md) | 各业务服务与 HPA 部署模板。 |
| [`shared/`](shared/_TREE.md) | 共享模板 helper。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
