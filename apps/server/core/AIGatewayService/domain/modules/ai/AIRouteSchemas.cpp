#include "modules/ai/AIRouteModule.hpp"

#include "services/ai/AIPublicDtos.hpp"

import memochat.ai.route_schema_algorithms;

namespace memochat::gate::modules::ai
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> AIRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;
    namespace modules = memochat::ai::route_schema::modules;

    return {
        MakeRouteSchema<memochat::gate::services::ai::AIRegisterApiProviderRequestDto,
                        memochat::gate::services::ai::AIRegisterApiProviderResponseDto>(
            modules::PostMethod(),
            modules::RegisterApiProviderPath(),
            modules::RegisterApiProviderRouteName(),
            modules::RegisterApiProviderRequestTypeName(),
            modules::RegisterApiProviderResponseTypeName()),
        MakeRouteSchema<memochat::gate::services::ai::AIDeleteApiProviderRequestDto,
                        memochat::gate::services::ai::AIDeleteApiProviderResponseDto>(
            modules::PostMethod(),
            modules::DeleteApiProviderPath(),
            modules::DeleteApiProviderRouteName(),
            modules::DeleteApiProviderRequestTypeName(),
            modules::DeleteApiProviderResponseTypeName()),
        MakeRouteSchema<memochat::gate::services::ai::AIKbUploadRequestDto,
                        memochat::gate::services::ai::AIKbUploadResponseDto>(modules::PostMethod(),
                                                                             modules::KbUploadPath(),
                                                                             modules::KbUploadRouteName(),
                                                                             modules::KbUploadRequestTypeName(),
                                                                             modules::KbUploadResponseTypeName()),
        MakeRouteSchema<memochat::gate::services::ai::AIKbSearchRequestDto,
                        memochat::gate::services::ai::AIKbSearchResponseDto>(modules::PostMethod(),
                                                                             modules::KbSearchPath(),
                                                                             modules::KbSearchRouteName(),
                                                                             modules::KbSearchRequestTypeName(),
                                                                             modules::KbSearchResponseTypeName()),
        MakeRouteSchema<memochat::gate::services::ai::AIKbDeleteRequestDto,
                        memochat::gate::services::ai::AISimpleResponseDto>(modules::PostMethod(),
                                                                           modules::KbDeletePath(),
                                                                           modules::KbDeleteRouteName(),
                                                                           modules::KbDeleteRequestTypeName(),
                                                                           modules::SimpleResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::ai
