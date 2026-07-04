#include "GateDomainServer.hpp"
#include "GateRouteProfileRegistrar.hpp"

// CallGatewayServer — call signaling domain peeled off GateServer (gateserver
// split Phase 4). Serves /healthz, /readyz and /api/call/*. Owns memo call data
// (Postgres) + Redis call:* keys + LiveKit token issuance. It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(memochat::gate::profiles::RegisterCall,
                               "CallGatewayServer",
                               "CallGateway",
                               /*default_port=*/8097,
                               /*init_aws=*/false);
}
