#include "GateDomainServer.h"

// R18GatewayServer — R18 source domain peeled off GateServer (gateserver split
// Phase 4). Serves /healthz, /readyz and /api/r18/*. Owns memo r18 data + Redis
// r18 keys via R18SourceService. Opt-in in topology (default OFF).
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::R18,
                               "R18GatewayServer",
                               "R18Gateway",
                               /*default_port=*/8098,
                               /*init_aws=*/false);
}
