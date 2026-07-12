# persistence/ 目录树

> ChatServer 持久化层：PostgreSQL（消息/对话/群/用户分文件 DAO）、Mongo、Redis（在线路由/关系引导缓存/分布式锁）以及消息/关系/会话仓储与 Outbox 服务。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | ChatServer 持久化层 C++ module interface 分组。 |
| [`lua/`](lua/_TREE.md) | Redis Lua 脚本，提供分布式锁释放、计数下限扣减和在线路由修复等原子操作。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatMessageRepository.cpp` | 消息仓储实现 |
| `ChatMessageRepository.hpp` | 消息仓储声明 |
| `ChatOutboxRepairScheduler.cpp` | Outbox 修复调度适配器实现，将领域端口调用转发到 Postgres 持久化 |
| `ChatOutboxRepairScheduler.hpp` | Outbox 修复调度适配器声明 |
| `ChatOutboxService.cpp` | 消息 Outbox（可靠投递）服务实现 |
| `ChatOutboxService.hpp` | Outbox 服务声明 |
| `ChatRelationRepository.cpp` | 关系仓储实现 |
| `ChatRelationRepository.hpp` | 关系仓储声明 |
| `ChatSessionRepository.cpp` | 会话仓储实现 |
| `ChatSessionRepository.hpp` | 会话仓储声明 |
| `DistLock.cpp` | 分布式锁实现 |
| `DistLock.hpp` | 分布式锁声明 |
| `MongoDao.cpp` | MongoDB 数据访问实现 |
| `MongoDao.hpp` | MongoDB DAO 声明 |
| `MongoMgr.cpp` | MongoDB 管理器和 DAO 转发实现 |
| `MongoMgr.hpp` | MongoDB 管理声明 |
| `OnlineRouteSessionSupport.cpp` | 在线路由存储隔离 CSession 会话 ID 读取的非 module helper |
| `OnlineRouteSessionSupport.hpp` | 在线路由会话 ID 读取 helper 声明 |
| `PostgresDao.cpp` | Postgres DAO 主实现 |
| `PostgresDaoDialogs.cpp` | Postgres 对话表 DAO 实现 |
| `PostgresDaoGroupMessages.cpp` | Postgres 群消息表 DAO 实现 |
| `PostgresDaoGroups.cpp` | Postgres 群表 DAO 实现 |
| `PostgresDaoPrivateMessages.cpp` | Postgres 私聊消息表 DAO 实现 |
| `PostgresDaoUsers.cpp` | Postgres 用户表 DAO 实现 |
| `PostgresDao.hpp` | Postgres DAO 声明 |
| `PostgresDaoUtil.hpp` | Postgres DAO 公共工具 |
| `PostgresMgr.cpp` | Postgres 连接管理实现 |
| `PostgresMgr.hpp` | Postgres 管理声明 |
| `RedisMgr.cpp` | Redis 连接管理实现 |
| `RedisMgr.hpp` | Redis 管理声明 |
| `RedisOnlineRouteStore.cpp` | Redis 在线路由存储实现 |
| `RedisOnlineRouteStore.hpp` | 在线路由存储声明 |
| `RedisRelationBootstrapCache.cpp` | Redis 关系引导缓存实现 |
| `RedisRelationBootstrapCache.hpp` | 关系引导缓存声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
