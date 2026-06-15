#include "GateDomainServer.h"

// MediaGatewayServer — media upload/download domain peeled off GateServer
// (gateserver split Phase 4). Serves /healthz, /readyz and the media routes
// (/upload_media*, /media/download). Owns memo media data + MinIO buckets; needs
// AWS SDK init for S3 storage. Opt-in in the runtime topology (default OFF).
int main()
{
    return RunGateDomainServer(LogicSystem::RouteProfile::Media,
                               "MediaGatewayServer",
                               "MediaGateway",
                               /*default_port=*/8094,
                               /*init_aws=*/true);
}
