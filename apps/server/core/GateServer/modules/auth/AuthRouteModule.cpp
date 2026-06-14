#include "modules/auth/AuthRouteModule.h"

#include "services/auth/AuthService.h"
#include "routing/RouteRegistry.h"

namespace memochat::gate::modules::auth
{

void AuthRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register(
        "POST",
        "/get_varifycode",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleGetVarifyCode(request, response);
        });
    registry.Register(
        "POST",
        "/user_register",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleUserRegister(request, response);
        });
    registry.Register(
        "POST",
        "/reset_pwd",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleResetPwd(request, response);
        });
    registry.Register(
        "POST",
        "/user_login",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::auth::AuthService::Instance().HandleUserLogin(request, response);
        });
}

} // namespace memochat::gate::modules::auth
