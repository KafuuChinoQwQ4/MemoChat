# lua/ 目录树

> ChatServer 持久化层 Redis Lua 脚本目录，存放需要 Redis 原子执行的辅助逻辑。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `dist_lock_release.lua` | 分布式锁释放脚本，校验锁值后删除锁键。 |
| `hash_decrement_floor.lua` | Hash 字段扣减脚本，保证结果不低于下限。 |
| `repair_online_route.lua` | 在线路由修复脚本，按会话信息原子修复 Redis 路由状态。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
