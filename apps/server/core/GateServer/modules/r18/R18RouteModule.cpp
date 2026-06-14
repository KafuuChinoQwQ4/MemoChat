#include "modules/r18/R18RouteModule.h"

#include "routing/RouteRegistry.h"
#include "services/r18/R18Service.h"

namespace memochat::gate::modules::r18
{

void R18RouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register(
        "GET",
        "/api/r18/sources",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleListSources(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/source/import",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleImportSource(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/source/enable",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleEnableSource(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/source/disable",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleDisableSource(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/search",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleSearch(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/comic/detail",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleComicDetail(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/chapter/pages",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleChapterPages(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/favorite/toggle",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleFavoriteToggle(request, response);
        });
    registry.Register(
        "POST",
        "/api/r18/history/update",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleHistoryUpdate(request, response);
        });
    registry.Register(
        "GET",
        "/api/r18/history",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleHistory(request, response);
        });
    registry.Register(
        "GET",
        "/api/r18/image",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleImage(request, response);
        });
}

} // namespace memochat::gate::modules::r18
