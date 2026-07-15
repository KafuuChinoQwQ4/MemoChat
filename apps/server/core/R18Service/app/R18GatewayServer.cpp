#include "CacheReadinessProbes.hpp"
#include "GateDomainServer.hpp"
#include "GateRouteProfileRegistrar.hpp"
#include "PersistenceReadinessProbes.hpp"

// R18GatewayServer — R18 source domain peeled off GateServer (gateserver split
// Phase 4). Serves /healthz, /readyz and /api/r18/*. R18SourceService owns its
// source files/Redis state; account-owned access policy is read from Postgres.
int main()
{
    return RunGateDomainServer(
        memochat::gate::profiles::RegisterR18,
        "R18GatewayServer",
        "R18Gateway",
        /*default_port=*/8098,
        /*init_aws=*/false,
        {},
        {},
        {memochat::gate::persistence::PostgresReadinessProbe(), memochat::gate::cache::RedisReadinessProbe()});
}
