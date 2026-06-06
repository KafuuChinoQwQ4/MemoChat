#pragma once

#include <string>

class IRelationServiceConfig
{
public:
    virtual ~IRelationServiceConfig() = default;

    virtual std::string RelationServiceBackend() const = 0;
    virtual std::string RelationServiceEndpoint() const = 0;
};
