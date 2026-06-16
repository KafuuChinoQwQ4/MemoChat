#pragma once

#include "routing/GateRequest.h"
#include "routing/GateResponse.h"

namespace memochat::gate::services::auth
{

class AuthService
{
public:
    static AuthService& Instance();

    bool HandleGetVarifyCode(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);
    bool HandleUserRegister(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleResetPwd(const memochat::gate::routing::GateRequest& request,
                        memochat::gate::routing::GateResponse& response);
    bool HandleUserLogin(const memochat::gate::routing::GateRequest& request,
                         memochat::gate::routing::GateResponse& response);

private:
    AuthService() = default;
};

} // namespace memochat::gate::services::auth
