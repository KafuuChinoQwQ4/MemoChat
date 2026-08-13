#pragma once

#include "ports/IRelationCommandService.hpp"
#include "ports/IRelationQueryService.hpp"

class IRelationService
    : public IRelationQueryService
    , public IRelationCommandService
{
public:
    virtual ~IRelationService() = default;

    virtual bool AreUsersFriends(int uid, int peer_uid) = 0;

    // Used only by the authenticated internal CheckHealth RPC. This reports
    // validated startup readiness; adapters without a bounded background
    // dependency monitor remain fail-closed by default.
    virtual bool CheckHealth(std::string* error)
    {
        if (error != nullptr)
        {
            *error = "relation service health check is not implemented";
        }
        return false;
    }
};
