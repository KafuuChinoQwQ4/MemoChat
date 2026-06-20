#include "modules/r18/R18RouteModule.h"

#include "r18/R18PublicDtos.h"

namespace memochat::gate::modules::r18
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> R18RouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<memochat::r18::R18SourceToggleRequestDto, memochat::r18::R18SourceToggleResponseDto>(
            "POST",
            "/api/r18/source/enable",
            "r18.source.enable",
            "R18SourceToggleRequestDto",
            "R18SourceToggleResponseDto"),
        MakeRouteSchema<memochat::r18::R18SourceToggleRequestDto, memochat::r18::R18SourceToggleResponseDto>(
            "POST",
            "/api/r18/source/disable",
            "r18.source.disable",
            "R18SourceToggleRequestDto",
            "R18SourceToggleResponseDto"),
        MakeRouteSchema<memochat::r18::R18FavoriteToggleRequestDto, memochat::r18::R18FavoriteToggleResponseDto>(
            "POST",
            "/api/r18/favorite/toggle",
            "r18.favorite.toggle",
            "R18FavoriteToggleRequestDto",
            "R18FavoriteToggleResponseDto"),
        MakeRouteSchema<memochat::r18::R18HistoryUpdateRequestDto, memochat::r18::R18HistoryUpdateResponseDto>(
            "POST",
            "/api/r18/history/update",
            "r18.history.update",
            "R18HistoryUpdateRequestDto",
            "R18HistoryUpdateResponseDto"),
    };
}

} // namespace memochat::gate::modules::r18
