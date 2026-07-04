#pragma once

#include "ports/IRelationCommandService.hpp"
#include "ports/IRelationQueryService.hpp"

class IRelationService
    : public IRelationQueryService
    , public IRelationCommandService
{
public:
    virtual ~IRelationService() = default;
};
