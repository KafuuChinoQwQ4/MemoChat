#include "GateDomainServer.h"

// AccountServer — the memo_account data owner (D-ACCOUNT), peeled off GateServer
// (gateserver split Phase 5). Serves /healthz, /readyz, /user_update_profile
// (the profile aggregate). Register/Login go THROUGH account-core for account
// data; AccountServer owns the profile write path + identity caches
// (ubaseinfo_<uid>, nameinfo_<name>). Emits cache-invalidate side effects
// (init_async=true). It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::Account,
                               "AccountServer",
                               "Account",
                               /*default_port=*/8103,
                               /*init_aws=*/false,
                               /*init_async=*/true);
}
