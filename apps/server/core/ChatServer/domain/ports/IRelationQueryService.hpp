#pragma once

#include "json/GlazeCompat.hpp"

class IRelationQueryService
{
public:
    virtual ~IRelationQueryService() = default;

    virtual void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) = 0;
    virtual void BuildDialogListJson(int uid, memochat::json::JsonValue& out) = 0;
};
