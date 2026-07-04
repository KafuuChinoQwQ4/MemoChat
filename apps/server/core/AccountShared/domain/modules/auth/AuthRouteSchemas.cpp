#include "modules/auth/AuthRouteModule.hpp"

#include "AuthPublicDtos.hpp"

import memochat.account.auth_route_schema_algorithms;

namespace memochat::gate::modules::auth
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> AuthRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<gateauthsupport::AuthRegisterRequestDto, gateauthsupport::AuthRegisterResponseDto>(
            memochat::account::auth_route_schema::modules::PostMethod(),
            memochat::account::auth_route_schema::modules::RegisterPath(),
            memochat::account::auth_route_schema::modules::RegisterRouteName(),
            memochat::account::auth_route_schema::modules::RegisterRequestTypeName(),
            memochat::account::auth_route_schema::modules::RegisterResponseTypeName()),
        MakeRouteSchema<gateauthsupport::AuthResetPasswordRequestDto, gateauthsupport::AuthResetPasswordResponseDto>(
            memochat::account::auth_route_schema::modules::PostMethod(),
            memochat::account::auth_route_schema::modules::ResetPasswordPath(),
            memochat::account::auth_route_schema::modules::ResetPasswordRouteName(),
            memochat::account::auth_route_schema::modules::ResetPasswordRequestTypeName(),
            memochat::account::auth_route_schema::modules::ResetPasswordResponseTypeName()),
        MakeRouteSchema<gateauthsupport::AuthLoginRequestDto, gateauthsupport::AuthLoginResponseDto>(
            memochat::account::auth_route_schema::modules::PostMethod(),
            memochat::account::auth_route_schema::modules::LoginPath(),
            memochat::account::auth_route_schema::modules::LoginRouteName(),
            memochat::account::auth_route_schema::modules::LoginRequestTypeName(),
            memochat::account::auth_route_schema::modules::LoginResponseTypeName()),
        MakeRouteSchema<gateauthsupport::AuthRefreshRequestDto, gateauthsupport::AuthLoginResponseDto>(
            memochat::account::auth_route_schema::modules::PostMethod(),
            memochat::account::auth_route_schema::modules::RefreshPath(),
            memochat::account::auth_route_schema::modules::RefreshRouteName(),
            memochat::account::auth_route_schema::modules::RefreshRequestTypeName(),
            memochat::account::auth_route_schema::modules::RefreshResponseTypeName()),
        MakeRouteSchema<gateauthsupport::AuthLogoutRequestDto, gateauthsupport::AuthLogoutResponseDto>(
            memochat::account::auth_route_schema::modules::PostMethod(),
            memochat::account::auth_route_schema::modules::LogoutPath(),
            memochat::account::auth_route_schema::modules::LogoutRouteName(),
            memochat::account::auth_route_schema::modules::LogoutRequestTypeName(),
            memochat::account::auth_route_schema::modules::LogoutResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::auth
