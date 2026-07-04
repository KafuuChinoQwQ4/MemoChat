#include "modules/r18/R18RouteModule.hpp"

#include "r18/R18PublicDtos.hpp"

import memochat.r18.route_schema_algorithms;

namespace memochat::gate::modules::r18
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> R18RouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<memochat::r18::R18SourceToggleRequestDto, memochat::r18::R18SourceToggleResponseDto>(
            memochat::r18::route_schema::modules::PostMethod(),
            memochat::r18::route_schema::modules::SourceEnablePath(),
            memochat::r18::route_schema::modules::SourceEnableRouteName(),
            memochat::r18::route_schema::modules::SourceToggleRequestTypeName(),
            memochat::r18::route_schema::modules::SourceToggleResponseTypeName()),
        MakeRouteSchema<memochat::r18::R18SourceToggleRequestDto, memochat::r18::R18SourceToggleResponseDto>(
            memochat::r18::route_schema::modules::PostMethod(),
            memochat::r18::route_schema::modules::SourceDisablePath(),
            memochat::r18::route_schema::modules::SourceDisableRouteName(),
            memochat::r18::route_schema::modules::SourceToggleRequestTypeName(),
            memochat::r18::route_schema::modules::SourceToggleResponseTypeName()),
        MakeRouteSchema<memochat::r18::R18FavoriteToggleRequestDto, memochat::r18::R18FavoriteToggleResponseDto>(
            memochat::r18::route_schema::modules::PostMethod(),
            memochat::r18::route_schema::modules::FavoriteTogglePath(),
            memochat::r18::route_schema::modules::FavoriteToggleRouteName(),
            memochat::r18::route_schema::modules::FavoriteToggleRequestTypeName(),
            memochat::r18::route_schema::modules::FavoriteToggleResponseTypeName()),
        MakeRouteSchema<memochat::r18::R18HistoryUpdateRequestDto, memochat::r18::R18HistoryUpdateResponseDto>(
            memochat::r18::route_schema::modules::PostMethod(),
            memochat::r18::route_schema::modules::HistoryUpdatePath(),
            memochat::r18::route_schema::modules::HistoryUpdateRouteName(),
            memochat::r18::route_schema::modules::HistoryUpdateRequestTypeName(),
            memochat::r18::route_schema::modules::HistoryUpdateResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::r18
