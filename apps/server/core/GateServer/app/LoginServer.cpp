#include "GateDomainServer.h"

// LoginServer — authentication, peeled off GateServer (gateserver split
// Phase 5). Serves /healthz, /readyz, /user_login. Reaches account data only via
// account-core (AccountPersistence) per D-ACCOUNT; issues the HTTP session token
// (Redis utoken_<uid>) + HMAC login ticket (common/auth, ChatAuth.HmacSecret) —
// the D-TOKEN contract ChatServer validates locally. Emits audit side effects
// (init_async=true). Opt-in in the runtime topology (default OFF).
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::Login,
                               "LoginServer",
                               "Login",
                               /*default_port=*/8102,
                               /*init_aws=*/false,
                               /*init_async=*/true);
}
