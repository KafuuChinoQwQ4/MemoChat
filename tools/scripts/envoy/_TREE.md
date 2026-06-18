# envoy/ 目录树

> 针对 Envoy 网关的运行时验证脚本，覆盖鉴权限流、故障转移、网关路由、Loki 日志与 metrics。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_envoy_auth_limit.ps1` | 验证 Envoy 鉴权与限流行为 |
| `test_envoy_failover.ps1` | 验证 Envoy 上游故障转移 |
| `test_envoy_gateway.ps1` | 验证 Envoy 网关路由 |
| `test_envoy_loki.ps1` | 验证 Envoy 访问日志接入 Loki |
| `test_envoy_metrics.ps1` | 验证 Envoy metrics 暴露 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
