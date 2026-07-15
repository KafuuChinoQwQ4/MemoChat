# persistence/ 目录树

> Gate 的数据访问层：MongoDB 与 PostgreSQL 的连接管理与 DAO。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | Postgres/Mongo 持久化层的 C++ module 接口分组 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallSessionTypes.h` | 通话用户资料与通话会话持久化结构声明。 |
| `MomentTypes.h` | 朋友圈动态、内容、点赞和评论的轻量持久化结构声明。 |
| `MongoDao.cpp` | MongoDB 数据访问对象实现，导入轻量算法 module 处理启用、默认 collection 与 moment_id guard。 |
| `MongoDao.h` | Mongo DAO 接口声明。 |
| `MongoMgr.cpp` | MongoDB 连接管理实现。 |
| `MongoMgr.h` | Mongo 连接管理接口声明。 |
| `PersistenceReadinessProbes.cpp` | Postgres/Mongo 启动就绪探针工厂实现（仅本切片引用具体 Mgr 单例）。 |
| `PersistenceReadinessProbes.hpp` | 持久化层就绪探针工厂声明，供各服务 main() 注入到共享启动流程。 |
| `PostgresDao.cpp` | PostgreSQL 通用 DAO 实现，包含媒体资产/下载 grant、通话和朋友圈持久化。 |
| `PostgresDao.hpp` | Postgres DAO 接口声明。 |
| `PostgresDaoAccount.cpp` | Postgres 账户相关 DAO 实现，包含密码更新、R18 成人确认/撤销不变量及 refresh token 轮换、吊销与活跃复查。 |
| `PostgresMgr.cpp` | PostgreSQL 连接管理实现。 |
| `PostgresMgr.hpp` | Postgres 连接管理接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
