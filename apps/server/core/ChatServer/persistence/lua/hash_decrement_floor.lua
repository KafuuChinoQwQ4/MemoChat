-- hash_decrement_floor.lua
-- 原子地对 Hash 字段执行有下限（最小为 0）的递减操作。
-- 解决问题：HGET + 判断 + HSET 三步非原子，并发时计数可能变负数。
--
-- KEYS[1] : Hash 键名（如 LOGIN_COUNT）
-- ARGV[1] : Hash 字段名（如服务器名称）
-- 返回    : 递减前的原始值（0 表示已到底，不再递减）

local v = tonumber(redis.call('HGET', KEYS[1], ARGV[1]) or '0')
if v > 0 then
    redis.call('HSET', KEYS[1], ARGV[1], tostring(v - 1))
end
return v
