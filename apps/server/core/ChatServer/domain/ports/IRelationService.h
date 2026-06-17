#pragma once

#include "ports/IRelationCommandService.h"
#include "ports/IRelationQueryService.h"

class IRelationService
    : public IRelationQueryService
    , public IRelationCommandService
{
public:
    virtual ~IRelationService() = default;
};
