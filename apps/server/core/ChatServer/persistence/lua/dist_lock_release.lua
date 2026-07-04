-- dist_lock_release.lua
-- 原子地释放分布式锁：先验证锁的持有者是否匹配，再删除。
-- 解决问题：GET + DEL 两步非原子，可能误删别的线程持有的锁。
--
-- KEYS[1] : 锁的键名（如 "lock:count"）
-- ARGV[1] : 锁持有者的唯一标识符（UUID）
-- 返回    : 1 表示成功释放，0 表示锁不属于当前持有者

if redis.call('GET', KEYS[1]) == ARGV[1] then
    return redis.call('DEL', KEYS[1])
else
    return 0
end
