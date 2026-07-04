#include "RedisPipelines.hpp"
#include "RedisMgr.hpp"
#include "logging/Logger.hpp"

import memochat.gate.redis_pipeline_algorithms;

namespace redis_pipeline_modules = memochat::gate::redis_pipeline::modules;

namespace
{

std::string BuildLoginCacheKey(const std::string& email)
{
    return std::string(redis_pipeline_modules::LoginProfilePrefix()) + email;
}

std::string BuildLoginCacheUidKey(int uid)
{
    return std::string(redis_pipeline_modules::LoginProfileUidPrefix()) + std::to_string(uid);
}

std::string BuildTokenKey(int uid)
{
    return std::string(redis_pipeline_modules::UserTokenPrefix()) + std::to_string(uid);
}

} // namespace

namespace gateredis
{

bool WarmupLoginCache(const std::string& email, const std::string& pwd)
{
    redisContext* ctx = RedisMgr::GetInstance()->getRawConnection();
    if (!ctx)
    {
        return false;
    }

    const std::string cache_key = BuildLoginCacheKey(email);
    const std::string token_key = BuildTokenKey(0);
    const std::string uid_key = BuildLoginCacheUidKey(0);

    const char* argv[6] = {"MGET", cache_key.c_str(), token_key.c_str(), uid_key.c_str(), nullptr, nullptr};
    size_t argvlen[6] = {4, cache_key.size(), 0, 0, 0, 0};

    // Only do MGET for cache key (email-based lookup)
    // Skip token and uid keys since we don't know uid yet
    auto reply = (redisReply*) redisCommand(ctx, "GET %s", cache_key.c_str());
    bool ok = false;
    if (reply && reply->type == REDIS_REPLY_STRING)
    {
        ok = true;
    }
    if (reply)
        freeReplyObject(reply);
    RedisMgr::GetInstance()->returnConnection(ctx);
    return ok;
}

} // namespace gateredis
