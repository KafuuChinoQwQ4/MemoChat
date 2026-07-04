#include "GateDomainServer.hpp"
#include "GateAsyncSideEffects.hpp"
#include "GateRouteProfileRegistrar.hpp"

// RegisterServer — account creation + recovery, peeled off GateServer
// (gateserver split Phase 5). Serves /healthz, /readyz, /get_varifycode,
// /user_register, /reset_pwd. Reaches account data only via account-core
// (AccountPersistence), per D-ACCOUNT — it never touches the users table
// directly. Emits audit + cache-invalidate side effects through account-core.
// It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(
        memochat::gate::profiles::RegisterRegister,
        "RegisterServer",
        "Register",
        /*default_port=*/8101,
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
