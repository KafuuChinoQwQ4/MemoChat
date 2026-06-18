# MomentsService/ 目录树

> 朋友圈网关微服务根：负责动态发布、点赞、评论等 Moments 业务，独立成 MomentsGateway 服务进程，并跨库读取账号/关系数据。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | MomentsGateway 服务进程入口 |
| [`clients/`](clients/_TREE.md) | 关系服务等下游客户端 |
| [`domain/`](domain/_TREE.md) | 朋友圈业务领域层：路由、profile 与服务 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | MomentsService 的构建定义 |
| `momentsgateway.ini` | 朋友圈网关运行期配置（Postgres/AccountPostgres/关系查询/Mongo 等段） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
