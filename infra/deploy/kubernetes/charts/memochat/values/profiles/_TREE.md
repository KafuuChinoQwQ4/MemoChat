# profiles/ 目录树

> 生产可选能力的最小 Helm values 覆盖；先加载 `prod.yaml` 与 `base.yaml`，再按需组合能力 profiles。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `base.yaml` | 明确关闭 CallGateway、R18Gateway 与 observability。 |
| `calls.yaml` | 仅启用 CallGateway，并显式启用 relation-query 依赖。 |
| `observability.yaml` | 仅启用 OTel Collector 与监控资源。 |
| `r18.yaml` | 仅启用 R18Gateway，保留持久化与加密 master key 门禁。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
