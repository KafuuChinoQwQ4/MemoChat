#pragma once

#include "ports/IRelationServiceConfig.h"

class RelationServiceConfig : public IRelationServiceConfig
{
public:
    std::string RelationServiceBackend() const override;
    std::string RelationServiceEndpoint() const override;
};
