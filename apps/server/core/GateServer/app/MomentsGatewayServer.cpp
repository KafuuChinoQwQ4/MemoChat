#include "GateDomainServer.h"

// MomentsGatewayServer — moments feed domain peeled off GateServer (gateserver
// split Phase 4). Serves /healthz, /readyz and /api/moments/*. Owns memo moments
// data (Postgres) + Mongo moment content. Opt-in in topology (default OFF).
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::Moments,
                               "MomentsGatewayServer",
                               "MomentsGateway",
                               /*default_port=*/8099,
                               /*init_aws=*/false);
}
