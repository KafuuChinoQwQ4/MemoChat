#include "GateDomainServer.h"
#include "GateAsyncSideEffects.h"
#include "GateRouteProfileRegistrar.h"

// LoginServer — authentication, peeled off GateServer (gateserver split
// Phase 5). Serves /healthz, /readyz, /user_login. Reaches account data only via
// account-core (AccountPersistence) per D-ACCOUNT; issues the HTTP session token
// (Redis utoken_<uid>) + HMAC login ticket (common/auth, ChatAuth.HmacSecret) —
// the D-TOKEN contract ChatServer validates locally. Emits audit side effects
// through account-core side-effect hooks. It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(
        memochat::gate::profiles::RegisterLogin,
        "LoginServer",
        "Login",
        /*default_port=*/8102,
        /*init_aws=*/false,
        []()
        {
            GateAsyncSideEffects::Instance().Start();
        },
        []()
        {
            GateAsyncSideEffects::Instance().Stop();
        });
}
