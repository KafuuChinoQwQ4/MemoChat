#include "RedisRelationBootstrapCache.hpp"

#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"

import memochat.chat.relation_bootstrap_cache_algorithms;

#include <memory>
#include <string>

namespace
{
namespace cache_modules = memochat::chat::persistence::relation_bootstrap_cache::modules;

constexpr int kDefaultRelationBootstrapCacheTtlSec = 15;
constexpr int kMinimumRelationBootstrapCacheTtlSec = 1;

bool RelationBootstrapCachePayloadIsCurrent(const memochat::json::JsonValue& cached)
{
    return cache_modules::IsCurrentSchemaVersion(cached.isObject(), cached.get("schema_version", 0).asInt());
}
} // namespace

std::string RedisRelationBootstrapCache::CacheKey(int uid) const
{
    return std::string("relation_bootstrap_v") + std::to_string(cache_modules::SchemaVersion()) + "_" +
           std::to_string(uid);
}

int RedisRelationBootstrapCache::TtlSec() const
{
    auto& cfg = ConfigMgr::Inst();
    const auto raw = cfg.GetValue("RelationBootstrapCache", "TtlSec");
    if (raw.empty())
    {
        return cache_modules::SelectTtlSec(false,
                                           0,
                                           kDefaultRelationBootstrapCacheTtlSec,
                                           kMinimumRelationBootstrapCacheTtlSec);
    }
    try
    {
        return cache_modules::SelectTtlSec(true,
                                           std::stoi(raw),
                                           kDefaultRelationBootstrapCacheTtlSec,
                                           kMinimumRelationBootstrapCacheTtlSec);
    }
    catch (...)
    {
        return cache_modules::SelectTtlSec(false,
                                           0,
                                           kDefaultRelationBootstrapCacheTtlSec,
                                           kMinimumRelationBootstrapCacheTtlSec);
    }
}

bool RedisRelationBootstrapCache::TryAppend(int uid, memochat::json::JsonValue& out)
{
    if (!cache_modules::IsValidUid(uid))
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

    if (!RelationBootstrapCachePayloadIsCurrent(cached))
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
    if (!cache_modules::IsValidUid(uid) || !payload.isObject())
    {
        return;
    }

    memochat::json::JsonValue versioned_payload = payload;
    versioned_payload["schema_version"] = cache_modules::SchemaVersion();

    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    const auto json = memochat::json::writeString(builder, versioned_payload);
    if (json.empty())
    {
        return;
    }

    RedisMgr::GetInstance()->SetEx(CacheKey(uid), json, TtlSec());
}

void RedisRelationBootstrapCache::Invalidate(int uid)
{
    if (cache_modules::IsValidUid(uid))
    {
        RedisMgr::GetInstance()->Del(CacheKey(uid));
    }
}
