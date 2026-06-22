#include "modules/ai/AIRouteModule.h"

#include "services/ai/AIPublicDtos.h"

namespace memochat::gate::modules::ai
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> AIRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<memochat::gate::services::ai::AIRegisterApiProviderRequestDto,
                        memochat::gate::services::ai::AIRegisterApiProviderResponseDto>(
            "POST",
            "/ai/model/api/register",
            "ai.model.api.register",
            "AIRegisterApiProviderRequestDto",
            "AIRegisterApiProviderResponseDto"),
        MakeRouteSchema<memochat::gate::services::ai::AIDeleteApiProviderRequestDto,
                        memochat::gate::services::ai::AIDeleteApiProviderResponseDto>("POST",
                                                                                      "/ai/model/api/delete",
                                                                                      "ai.model.api.delete",
                                                                                      "AIDeleteApiProviderRequestDto",
                                                                                      "AIDeleteApiProviderResponseDto"),
        MakeRouteSchema<memochat::gate::services::ai::AIKbUploadRequestDto,
                        memochat::gate::services::ai::AIKbUploadResponseDto>("POST",
                                                                             "/ai/kb/upload",
                                                                             "ai.kb.upload",
                                                                             "AIKbUploadRequestDto",
                                                                             "AIKbUploadResponseDto"),
        MakeRouteSchema<memochat::gate::services::ai::AIKbSearchRequestDto,
                        memochat::gate::services::ai::AIKbSearchResponseDto>("POST",
                                                                             "/ai/kb/search",
                                                                             "ai.kb.search",
                                                                             "AIKbSearchRequestDto",
                                                                             "AIKbSearchResponseDto"),
        MakeRouteSchema<memochat::gate::services::ai::AIKbDeleteRequestDto,
                        memochat::gate::services::ai::AISimpleResponseDto>("POST",
                                                                           "/ai/kb/delete",
                                                                           "ai.kb.delete",
                                                                           "AIKbDeleteRequestDto",
                                                                           "AISimpleResponseDto"),
    };
}

} // namespace memochat::gate::modules::ai
