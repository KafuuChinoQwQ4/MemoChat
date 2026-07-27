# local/ 目录树

> 本地 Docker Compose 一键开发环境：主 compose、分组 compose、数据库初始化与可观测性栈配置。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`compose/`](compose/_TREE.md) | 按组件拆分的 compose 片段与 Envoy 配置。 |
| [`init/`](init/_TREE.md) | 数据库初始化脚本（Mongo/PostgreSQL）。 |
| [`observability/`](observability/_TREE.md) | 本地可观测性栈配置（Prometheus/Grafana/Loki/Tempo/OTel/Alertmanager）。 |
| [`provision/`](provision/_TREE.md) | 正式部署的幂等数据存储初始化脚本与最小权限策略。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `.env.release.example` | 区分核心值与可选 profile 值的 C++ 后端发布 Compose 无凭据环境变量模板。 |
| `README.md` | 本地环境使用说明。 |
| `docker-compose.yml` | 本地环境主 compose 编排。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
