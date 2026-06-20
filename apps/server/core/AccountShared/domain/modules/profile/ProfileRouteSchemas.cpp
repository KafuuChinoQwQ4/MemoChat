#include "modules/profile/ProfileRouteModule.h"

#include "AuthPublicDtos.h"

namespace memochat::gate::modules::profile
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> ProfileRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<gateauthsupport::ProfileUpdateRequestDto, gateauthsupport::ProfileUpdateResponseDto>(
            "POST",
            "/user_update_profile",
            "profile.user.update",
            "ProfileUpdateRequestDto",
            "ProfileUpdateResponseDto"),
        MakeRouteSchema<gateauthsupport::GetUserInfoRequestDto, gateauthsupport::UserInfoResponseDto>(
            "POST",
            "/get_user_info",
            "profile.user.info",
            "GetUserInfoRequestDto",
            "UserInfoResponseDto"),
    };
}

} // namespace memochat::gate::modules::profile
