export module memochat.moments.gateway_runtime_algorithms;

export namespace memochat::moments::gateway_runtime::modules
{
// Process/instance name passed to RunGateDomainServer for logging + telemetry.
const char* ServerName()
{
    return "MomentsGatewayServer";
}

// Logical gateway name used for route profile registration + config lookup.
const char* GatewayName()
{
    return "MomentsGateway";
}

// Default HTTP listen port for the moments feed gateway.
int DefaultListenPort()
{
    return 8099;
}

// The moments gateway does not initialize the AWS SDK at startup.
bool ShouldInitAws()
{
    return false;
}
} // namespace memochat::moments::gateway_runtime::modules
