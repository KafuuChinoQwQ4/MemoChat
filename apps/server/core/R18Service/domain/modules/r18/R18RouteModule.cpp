#include "modules/r18/R18RouteModule.hpp"

#include "routing/RouteRegistry.hpp"
#include "services/r18/R18Service.hpp"

import memochat.r18.route_registration_algorithms;

namespace memochat::gate::modules::r18
{

void R18RouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::r18::route_registration::modules;

    registry.Register(
        modules::GetMethod(),
        modules::R18AccessStatusPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleAccessStatus(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::R18AccessAttestPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleAccessAttest(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::SourcesPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleListSources(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SourceImportPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleImportSource(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SourceEnablePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleEnableSource(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SourceDisablePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleDisableSource(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SourceDeletePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleDeleteSource(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SearchPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleSearch(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::ComicDetailPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleComicDetail(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::ChapterPagesPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleChapterPages(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::FavoriteTogglePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleFavoriteToggle(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::HistoryUpdatePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleHistoryUpdate(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::HistoryPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleHistory(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::ImagePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleImage(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::AccountsPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleListAccounts(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::AccountSavePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleSaveAccount(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::AccountLoginPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleLoginAccount(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::AccountClearPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleClearAccount(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::CheckinPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::r18::R18Service::Instance().HandleCheckin(request, response);
        });
}

} // namespace memochat::gate::modules::r18
