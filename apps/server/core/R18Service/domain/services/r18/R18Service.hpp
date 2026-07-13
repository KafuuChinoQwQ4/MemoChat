#pragma once

#include "routing/GateRequest.hpp"
#include "routing/GateResponse.hpp"

namespace memochat::gate::services::r18
{

class R18Service
{
public:
    static R18Service& Instance();

    bool HandleAccessStatus(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleAccessAttest(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
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
    bool HandleListAccounts(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleSaveAccount(const memochat::gate::routing::GateRequest& request,
                           memochat::gate::routing::GateResponse& response);
    bool HandleLoginAccount(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleClearAccount(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);

private:
    R18Service() = default;
};

} // namespace memochat::gate::services::r18
