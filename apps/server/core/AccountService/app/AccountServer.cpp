#include "GateDomainServer.hpp"
#include "GateRouteProfileRegistrar.hpp"

int main()
{
    return RunGateDomainServer(memochat::gate::profiles::RegisterAccount,
                               "AccountServer",
                               "Account",
                               /*default_port=*/8103,
                               /*init_aws=*/false,
                               {},
                               {},
                               {.postgres = true, .redis = true});
}
