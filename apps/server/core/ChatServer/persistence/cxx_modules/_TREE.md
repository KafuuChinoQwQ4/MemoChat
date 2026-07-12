# cxx_modules/ 目录树

> ChatServer 持久化层的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatSessionRepository.cppm` | Chat 会话仓储重复登录锁 uid 与释放 guard 决策算法 module。 |
| `DistLock.cppm` | Redis 分布式锁 acquire/release reply 判定算法 module。 |
| `MessageRepository.cppm` | Chat 消息仓储 Postgres/Mongo 双写与读回退决策算法 module。 |
| `MongoDao.cppm` | MongoDao 启用、分页、游标、编辑和撤回 guard 决策算法 module。 |
| `MongoMgr.cppm` | MongoMgr DAO 转发表面数量契约算法 module。 |
| `OnlineRouteStore.cppm` | Redis 在线路由 uid、server-name、session 和 online-set guard 决策算法 module。 |
| `Outbox.cppm` | Chat outbox retry 次数、backoff 和终止重试决策算法 module。 |
| `PostgresDao.cppm` | PostgresDao 连接配置段回退与默认值决策算法 module。 |
| `PostgresDaoDialogs.cppm` | PostgresDao 会话列表、read-state、dialog meta 和群申请分页 guard 决策算法 module。 |
| `PostgresDaoGroupMessages.cppm` | PostgresDao 群消息保存、编辑、撤回、历史分页和查找 guard 决策算法 module。 |
| `PostgresDaoGroups.cppm` | PostgresDao 群管理创建、申请、角色、权限和生命周期 guard 决策算法 module。 |
| `PostgresDaoPrivateMessages.cppm` | PostgresDao 私聊消息读取、分页、编辑、撤回和 read-state guard 决策算法 module。 |
| `PostgresDaoUsers.cppm` | PostgresDao 用户公开 ID 和 uid guard 决策算法 module。 |
| `PostgresMgr.cppm` | Chat PostgresMgr DAO 初始化和析构 reset guard 决策算法 module。 |
| `RedisMgr.cppm` | Chat RedisMgr TTL、reply 类型和空锁释放 guard 决策算法 module。 |
| `RelationRepository.cppm` | Chat 关系仓储 Postgres 转发表面数量契约算法 module。 |
| `RelationBootstrapCache.cppm` | 关系引导缓存 schema、TTL 和有效 uid 决策算法 module。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
