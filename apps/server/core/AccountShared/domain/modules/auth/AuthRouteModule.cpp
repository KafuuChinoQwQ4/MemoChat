#include "modules/auth/AuthRouteModule.hpp"

#include "services/auth/AuthService.hpp"
#include "routing/RouteRegistry.hpp"

import memochat.account.auth_route_registration_algorithms;

namespace memochat::gate::modules::auth
{

void AuthRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    RegisterRegisterRoutes(registry);
    RegisterLoginRoutes(registry);
}

void AuthRouteModule::RegisterRegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::account::auth_route_registration::modules;

    registry.Register(
        modules::PostMethod(),
        modules::GetVarifyCodePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleGetVarifyCode(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::UserRegisterPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleUserRegister(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::ResetPasswordPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleResetPwd(request, response);
        });
}

void AuthRouteModule::RegisterLoginRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::account::auth_route_registration::modules;

    registry.Register(
        modules::PostMethod(),
        modules::UserLoginPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleUserLogin(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::AuthRefreshPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleAuthRefresh(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::AuthLogoutPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleAuthLogout(request, response);
        });
}

} // namespace memochat::gate::modules::auth
