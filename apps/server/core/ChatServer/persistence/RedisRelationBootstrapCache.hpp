#pragma once

#include "ports/IRelationBootstrapCache.hpp"

#include <string>

class RedisRelationBootstrapCache : public IRelationBootstrapCache
{
public:
    bool TryAppend(int uid, memochat::json::JsonValue& out) override;
    void Store(int uid, const memochat::json::JsonValue& payload) override;
    void Invalidate(int uid) override;

private:
    std::string CacheKey(int uid) const;
    int TtlSec() const;
};
