#pragma once

#include "ports/IRelationQueryServiceConfig.hpp"

class RelationQueryServiceConfig : public IRelationQueryServiceConfig
{
public:
    std::string RelationQueryServiceBackend() const override;
    std::string RelationQueryServiceEndpoint() const override;
};
