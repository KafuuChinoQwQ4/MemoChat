#pragma once

#include "json/GlazeCompat.h"

class IRelationBootstrapCache
{
public:
    virtual ~IRelationBootstrapCache() = default;

    virtual bool TryAppend(int uid, memochat::json::JsonValue& out) = 0;
    virtual void Store(int uid, const memochat::json::JsonValue& payload) = 0;
    virtual void Invalidate(int uid) = 0;
};
