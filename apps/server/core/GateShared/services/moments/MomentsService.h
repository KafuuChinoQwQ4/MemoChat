#pragma once

#include "routing/GateRequest.h"
#include "routing/GateResponse.h"

namespace memochat::gate::services::moments
{

class MomentsService
{
public:
    static MomentsService& Instance();

    bool HandlePublish(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response);
    bool HandleList(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response);
    bool HandleDetail(const memochat::gate::routing::GateRequest& request,
                      memochat::gate::routing::GateResponse& response);
    bool HandleDelete(const memochat::gate::routing::GateRequest& request,
                      memochat::gate::routing::GateResponse& response);
    bool HandleLike(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response);
    bool HandleComment(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response);
    bool HandleCommentList(const memochat::gate::routing::GateRequest& request,
                           memochat::gate::routing::GateResponse& response);
    bool HandleCommentLike(const memochat::gate::routing::GateRequest& request,
                           memochat::gate::routing::GateResponse& response);

private:
    MomentsService() = default;
};

} // namespace memochat::gate::services::moments
