#include "modules/profile/ProfileRouteModule.hpp"

#include "AuthPublicDtos.hpp"

import memochat.account.profile_route_schema_algorithms;

namespace memochat::gate::modules::profile
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> ProfileRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<gateauthsupport::ProfileUpdateRequestDto, gateauthsupport::ProfileUpdateResponseDto>(
            memochat::account::profile_route_schema::modules::PostMethod(),
            memochat::account::profile_route_schema::modules::UpdateProfilePath(),
            memochat::account::profile_route_schema::modules::UpdateProfileRouteName(),
            memochat::account::profile_route_schema::modules::UpdateProfileRequestTypeName(),
            memochat::account::profile_route_schema::modules::UpdateProfileResponseTypeName()),
        MakeRouteSchema<gateauthsupport::GetUserInfoRequestDto, gateauthsupport::UserInfoResponseDto>(
            memochat::account::profile_route_schema::modules::PostMethod(),
            memochat::account::profile_route_schema::modules::GetUserInfoPath(),
            memochat::account::profile_route_schema::modules::GetUserInfoRouteName(),
            memochat::account::profile_route_schema::modules::GetUserInfoRequestTypeName(),
            memochat::account::profile_route_schema::modules::UserInfoResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::profile
