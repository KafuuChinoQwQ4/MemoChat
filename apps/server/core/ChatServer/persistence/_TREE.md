# persistence/ 目录树

> ChatServer 持久化层：Postgres（消息/对话/群/用户分文件 DAO + 连接池）、Mongo、Redis（在线路由/关系引导缓存/分布式锁）以及消息/关系/会话仓储与 Outbox 服务。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | ChatServer 持久化层 C++ module interface 分组。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatMessageRepository.cpp` | 消息仓储实现 |
| `ChatMessageRepository.h` | 消息仓储声明 |
| `ChatOutboxService.cpp` | 消息 Outbox（可靠投递）服务实现 |
| `ChatOutboxService.h` | Outbox 服务声明 |
| `ChatRelationRepository.cpp` | 关系仓储实现 |
| `ChatRelationRepository.h` | 关系仓储声明 |
| `ChatSessionRepository.cpp` | 会话仓储实现 |
| `ChatSessionRepository.h` | 会话仓储声明 |
| `DistLock.cpp` | 分布式锁实现 |
| `DistLock.h` | 分布式锁声明 |
| `MongoDao.cpp` | MongoDB 数据访问实现 |
| `MongoDao.h` | MongoDB DAO 声明 |
| `MongoMgr.cpp` | MongoDB 管理器和 DAO 转发实现 |
| `MongoMgr.h` | MongoDB 管理声明 |
| `OnlineRouteSessionSupport.cpp` | 在线路由存储隔离 CSession 会话 ID 读取的非 module helper |
| `OnlineRouteSessionSupport.h` | 在线路由会话 ID 读取 helper 声明 |
| `PostgresDao.cpp` | Postgres DAO 主实现 |
| `PostgresDaoDialogs.cpp` | Postgres 对话表 DAO 实现 |
| `PostgresDaoGroupMessages.cpp` | Postgres 群消息表 DAO 实现 |
| `PostgresDaoGroups.cpp` | Postgres 群表 DAO 实现 |
| `PostgresDaoPrivateMessages.cpp` | Postgres 私聊消息表 DAO 实现 |
| `PostgresDaoUsers.cpp` | Postgres 用户表 DAO 实现 |
| `PostgresDaoUtil.h` | Postgres DAO 公共工具 |
| `PostgresMgr.cpp` | Postgres 连接管理实现 |
| `PostgresMgr.h` | Postgres 管理声明 |
| `PostgresPool.cpp` | Postgres 连接池实现 |
| `PostgresPool.h` | Postgres 连接池声明 |
| `RedisMgr.cpp` | Redis 连接管理实现 |
| `RedisMgr.h` | Redis 管理声明 |
| `RedisOnlineRouteStore.cpp` | Redis 在线路由存储实现 |
| `RedisOnlineRouteStore.h` | 在线路由存储声明 |
| `RedisRelationBootstrapCache.cpp` | Redis 关系引导缓存实现 |
| `RedisRelationBootstrapCache.h` | 关系引导缓存声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
