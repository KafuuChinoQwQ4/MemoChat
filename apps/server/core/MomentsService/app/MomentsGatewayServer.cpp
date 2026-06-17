#include "GateDomainServer.h"
#include "GateRouteProfileRegistrar.h"

// MomentsGatewayServer — moments feed domain peeled off GateServer (gateserver
// split Phase 4). Serves /healthz, /readyz and /api/moments/*. Owns memo moments
// data (Postgres) + Mongo moment content. It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(memochat::gate::profiles::RegisterMoments,
                               "MomentsGatewayServer",
                               "MomentsGateway",
                               /*default_port=*/8099,
                               /*init_aws=*/false);
}
