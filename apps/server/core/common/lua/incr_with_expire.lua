-- incr_with_expire.lua
-- 原子地对计数器执行 INCR，并仅在首次创建时设置 TTL。
-- 解决问题：INCR + EXPIRE 两步非原子，并发时可能丢失过期时间。
--
-- KEYS[1] : 计数器键名
-- ARGV[1] : 过期秒数（整数）
-- 返回    : 自增后的计数值

local count = redis.call('INCR', KEYS[1])
if count == 1 then
    redis.call('EXPIRE', KEYS[1], tonumber(ARGV[1]))
end
return count
