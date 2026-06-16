#include "GateDomainServer.h"

// RegisterServer — account creation + recovery, peeled off GateServer
// (gateserver split Phase 5). Serves /healthz, /readyz, /get_varifycode,
// /user_register, /reset_pwd. Reaches account data only via account-core
// (AccountPersistence), per D-ACCOUNT — it never touches the users table
// directly. Emits audit + cache-invalidate side effects (init_async=true).
// It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::Register,
                               "RegisterServer",
                               "Register",
                               /*default_port=*/8101,
                               /*init_aws=*/false,
                               /*init_async=*/true);
}
