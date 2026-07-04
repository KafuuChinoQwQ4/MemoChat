#pragma once

#include "ports/IRelationServiceConfig.hpp"

class RelationServiceConfig : public IRelationServiceConfig
{
public:
    std::string RelationServiceBackend() const override;
    std::string RelationServiceEndpoint() const override;
};
