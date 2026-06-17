#include "GateDomainServer.h"
#include "GateRouteProfileRegistrar.h"

// MediaGatewayServer — media upload/download domain peeled off GateServer
// (gateserver split Phase 4). Serves /healthz, /readyz and the media routes
// (/upload_media*, /media/download). Owns memo media data + MinIO buckets; needs
// AWS SDK init for S3 storage. It starts by default after Envoy cut-over.
int main()
{
    return RunGateDomainServer(memochat::gate::profiles::RegisterMedia,
                               "MediaGatewayServer",
                               "MediaGateway",
                               /*default_port=*/8094,
                               /*init_aws=*/true);
}
