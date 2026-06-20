#include "modules/auth/AuthRouteModule.h"

#include "AuthPublicDtos.h"

namespace memochat::gate::modules::auth
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> AuthRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<gateauthsupport::AuthRegisterRequestDto, gateauthsupport::AuthRegisterResponseDto>(
            "POST",
            "/user_register",
            "auth.user.register",
            "AuthRegisterRequestDto",
            "AuthRegisterResponseDto"),
        MakeRouteSchema<gateauthsupport::AuthResetPasswordRequestDto, gateauthsupport::AuthResetPasswordResponseDto>(
            "POST",
            "/reset_pwd",
            "auth.reset.password",
            "AuthResetPasswordRequestDto",
            "AuthResetPasswordResponseDto"),
    };
}

} // namespace memochat::gate::modules::auth
