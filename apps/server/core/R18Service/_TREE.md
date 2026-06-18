# R18Service/ 目录树

> R18 网关微服务根：负责 R18 内容源接入与分发业务，独立成 R18Gateway 服务进程。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | R18Gateway 服务进程入口 |
| [`domain/`](domain/_TREE.md) | R18 业务领域层：路由、profile 与服务 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | R18Service 的构建定义 |
| `r18gateway.ini` | R18 网关运行期配置（Postgres/Redis/鉴权等段） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
