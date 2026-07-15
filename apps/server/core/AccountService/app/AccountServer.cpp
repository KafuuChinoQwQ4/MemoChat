#include "CacheReadinessProbes.hpp"
#include "GateDomainServer.hpp"
#include "GateRouteProfileRegistrar.hpp"
#include "PersistenceReadinessProbes.hpp"

int main()
{
    return RunGateDomainServer(
        memochat::gate::profiles::RegisterAccount,
        "AccountServer",
        "Account",
        /*default_port=*/8103,
        /*init_aws=*/false,
        {},
        {},
        {memochat::gate::persistence::PostgresReadinessProbe(), memochat::gate::cache::RedisReadinessProbe()});
}
