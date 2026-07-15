#include "CacheReadinessProbes.hpp"
#include "GateDomainServer.hpp"
#include "GateRouteProfileRegistrar.hpp"
#include "PersistenceReadinessProbes.hpp"

// CallGatewayServer — call signaling domain peeled off GateServer (gateserver
// split Phase 4). Serves /healthz, /readyz and /api/call/*. Owns memo call data
// (Postgres) + Redis call:* keys + LiveKit token issuance. It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(
        memochat::gate::profiles::RegisterCall,
        "CallGatewayServer",
        "CallGateway",
        /*default_port=*/8097,
        /*init_aws=*/false,
        {},
        {},
        {memochat::gate::persistence::PostgresReadinessProbe(), memochat::gate::cache::RedisReadinessProbe()});
}
