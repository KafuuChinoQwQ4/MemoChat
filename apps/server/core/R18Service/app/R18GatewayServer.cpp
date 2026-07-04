#include "GateDomainServer.hpp"
#include "GateRouteProfileRegistrar.hpp"

// R18GatewayServer — R18 source domain peeled off GateServer (gateserver split
// Phase 4). Serves /healthz, /readyz and /api/r18/*. Owns memo r18 data + Redis
// r18 keys via R18SourceService. It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(memochat::gate::profiles::RegisterR18,
                               "R18GatewayServer",
                               "R18Gateway",
                               /*default_port=*/8098,
                               /*init_aws=*/false);
}
