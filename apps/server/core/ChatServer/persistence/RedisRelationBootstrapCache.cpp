#include "RedisRelationBootstrapCache.h"

#include "ConfigMgr.h"
#include "RedisMgr.h"

#include <algorithm>
#include <memory>
#include <string>

std::string RedisRelationBootstrapCache::CacheKey(int uid) const
{
    return std::string("relation_bootstrap_") + std::to_string(uid);
}

int RedisRelationBootstrapCache::TtlSec() const
{
    auto& cfg = ConfigMgr::Inst();
    const auto raw = cfg.GetValue("RelationBootstrapCache", "TtlSec");
    if (raw.empty())
    {
        return 15;
    }
    try
    {
        return std::max(1, std::stoi(raw));
    }
    catch (...)
    {
        return 15;
    }
}

bool RedisRelationBootstrapCache::TryAppend(int uid, memochat::json::JsonValue& out)
{
    if (uid <= 0)
    {
        return false;
    }

    std::string payload;
    if (!RedisMgr::GetInstance()->Get(CacheKey(uid), payload) || payload.empty())
    {
        return false;
    }

    memochat::json::JsonCharReaderBuilder builder;
    std::unique_ptr<memochat::json::JsonCharReader> reader(builder.newCharReader());
    memochat::json::JsonValue cached;
    std::string errors;
    if (!reader->parse(payload.data(), payload.data() + payload.size(), &cached, &errors) || !cached.isObject())
    {
        return false;
    }

    if (cached.isMember("apply_list"))
    {
        out["apply_list"] = cached["apply_list"];
    }
    if (cached.isMember("friend_list"))
    {
        out["friend_list"] = cached["friend_list"];
    }
    return true;
}

void RedisRelationBootstrapCache::Store(int uid, const memochat::json::JsonValue& payload)
{
    if (uid <= 0 || !payload.isObject())
    {
        return;
    }

    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    const auto json = memochat::json::writeString(builder, payload);
    if (json.empty())
    {
        return;
    }

    RedisMgr::GetInstance()->SetEx(CacheKey(uid), json, TtlSec());
}

void RedisRelationBootstrapCache::Invalidate(int uid)
{
    if (uid > 0)
    {
        RedisMgr::GetInstance()->Del(CacheKey(uid));
    }
}
