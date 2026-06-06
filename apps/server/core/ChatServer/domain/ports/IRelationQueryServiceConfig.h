#pragma once

#include <string>

class IRelationQueryServiceConfig
{
public:
    virtual ~IRelationQueryServiceConfig() = default;

    virtual std::string RelationQueryServiceBackend() const = 0;
    virtual std::string RelationQueryServiceEndpoint() const = 0;
};
