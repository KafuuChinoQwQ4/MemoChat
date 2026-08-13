# kubernetes/ 目录树

> 测试 infra/deploy/kubernetes 的 Helm Chart 契约，当前聚焦网关拆分相关编排。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_memochat_chart_gateway_split.py` | 校验 Helm Chart 中网关拆分、Call 账号库凭据与 relation-query 发布依赖契约。 |
| `test_memochat_chart_workload_security.py` | 对 Helm 渲染出的全部工作负载执行非 root、只读根文件系统与显式可写卷安全合约。 |
| `test_memochat_chart_profiles.py` | 结构化渲染生产 base、Calls、R18、observability profiles，验证可选资源与安全依赖互不隐式启用。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
