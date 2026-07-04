# cxx_modules/ 目录树

> GateShared 持久化层的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MongoDao.cppm` | Mongo DAO 启用、配置默认值、索引和动态内容 moment_id guard 算法 module。 |
| `MongoMgr.cppm` | Mongo 管理 facade 转发面契约(moment content 方法数)算法 module。 |
| `PostgresDaoAccount.cppm` | Postgres 账户 DAO 默认头像、public-id retry 与账户查询 guard 算法 module。 |
| `PostgresDao.cppm` | Postgres DAO 默认 schema/sslmode、连接池大小和账户库 section guard 算法 module。 |
| `PostgresMgr.cppm` | Postgres 管理 facade 转发面契约(账户/通话/媒体/朋友圈方法数与总数)算法 module。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
