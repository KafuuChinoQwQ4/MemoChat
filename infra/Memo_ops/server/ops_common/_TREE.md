# ops_common/ 目录树

> MemoOps 服务端公共逻辑库：监控/分析、告警、健康检查、存储仓储、时间窗口与快照采集等共享模块。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | Python 包标识。 |
| `alerts.py` | 告警规则与触发逻辑。 |
| `analytics.py` | 通用分析逻辑。 |
| `config.py` | 公共配置加载。 |
| `health_checks.py` | 各服务健康检查。 |
| `ingest.py` | 监控数据接入/写入。 |
| `loadtest_analytics.py` | 压测结果分析。 |
| `log_analytics.py` | 日志分析。 |
| `monitoring.py` | 指标监控聚合逻辑。 |
| `overview_analytics.py` | 总览数据分析。 |
| `repositories.py` | 数据访问仓储层。 |
| `schema.py` | 数据模型/schema 定义。 |
| `service_analytics.py` | 服务维度指标分析。 |
| `snapshot_collector.py` | 快照采集逻辑。 |
| `storage.py` | 存储后端封装。 |
| `time_windows.py` | 时间窗口/聚合区间工具。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
