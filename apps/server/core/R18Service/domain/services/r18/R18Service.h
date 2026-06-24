#pragma once

#include "routing/GateRequest.h"
#include "routing/GateResponse.h"

namespace memochat::gate::services::r18
{

class R18Service
{
public:
    static R18Service& Instance();

    bool HandleListSources(const memochat::gate::routing::GateRequest& request,
                           memochat::gate::routing::GateResponse& response);
    bool HandleImportSource(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleEnableSource(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleDisableSource(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);
    bool HandleDeleteSource(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleSearch(const memochat::gate::routing::GateRequest& request,
                      memochat::gate::routing::GateResponse& response);
    bool HandleComicDetail(const memochat::gate::routing::GateRequest& request,
                           memochat::gate::routing::GateResponse& response);
    bool HandleChapterPages(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleFavoriteToggle(const memochat::gate::routing::GateRequest& request,
                              memochat::gate::routing::GateResponse& response);
    bool HandleHistoryUpdate(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);
    bool HandleHistory(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response);
    bool HandleImage(const memochat::gate::routing::GateRequest& request,
                     memochat::gate::routing::GateResponse& response);

private:
    R18Service() = default;
};

} // namespace memochat::gate::services::r18
