# persistence/ 目录树

> Gate 的数据访问层：MongoDB 与 PostgreSQL 的连接管理与 DAO。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallSessionTypes.h` | 通话用户资料与通话会话持久化结构声明。 |
| `MomentTypes.h` | 朋友圈动态、内容、点赞和评论的轻量持久化结构声明。 |
| `MongoDao.cpp` | MongoDB 数据访问对象实现。 |
| `MongoDao.h` | Mongo DAO 接口声明。 |
| `MongoMgr.cpp` | MongoDB 连接管理实现。 |
| `MongoMgr.h` | Mongo 连接管理接口声明。 |
| `PostgresDao.cpp` | PostgreSQL 通用 DAO 实现。 |
| `PostgresDao.h` | Postgres DAO 接口声明。 |
| `PostgresDaoAccount.cpp` | Postgres 账户相关 DAO 实现。 |
| `PostgresMgr.cpp` | PostgreSQL 连接管理实现。 |
| `PostgresMgr.h` | Postgres 连接管理接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
