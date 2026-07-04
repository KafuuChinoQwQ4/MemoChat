-- repair_online_route.lua
-- 原子地写入用户上线路由的三条记录。
-- 解决问题：三步独立写操作，服务崩溃时会留下部分状态，导致路由不一致。
--
-- KEYS[1] : uid → server 键（如 "useripprefix:42"）
-- KEYS[2] : uid → session 键（如 "user_session:42"）
-- KEYS[3] : server 在线用户集合键（如 "server_online_users:chatserver1"）
-- ARGV[1] : server 名称
-- ARGV[2] : session ID
-- ARGV[3] : uid 字符串
-- 返回    : 1 表示成功

redis.call('SET',  KEYS[1], ARGV[1])
redis.call('SET',  KEYS[2], ARGV[2])
redis.call('SADD', KEYS[3], ARGV[3])
return 1
