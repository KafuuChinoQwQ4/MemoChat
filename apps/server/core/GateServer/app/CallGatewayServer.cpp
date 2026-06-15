#include "GateDomainServer.h"

// CallGatewayServer — call signaling domain peeled off GateServer (gateserver
// split Phase 4). Serves /healthz, /readyz and /api/call/*. Owns memo call data
// (Postgres) + Redis call:* keys + LiveKit token issuance. Opt-in (default OFF).
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::Call,
                               "CallGatewayServer",
                               "CallGateway",
                               /*default_port=*/8097,
                               /*init_aws=*/false);
}
