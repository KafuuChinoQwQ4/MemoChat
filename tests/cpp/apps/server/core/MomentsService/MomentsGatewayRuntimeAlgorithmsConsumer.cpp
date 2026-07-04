import memochat.moments.gateway_runtime_algorithms;

namespace memochat::tests::moments::gateway_runtime
{
const char* ServerName()
{
    return memochat::moments::gateway_runtime::modules::ServerName();
}

const char* GatewayName()
{
    return memochat::moments::gateway_runtime::modules::GatewayName();
}

int DefaultListenPort()
{
    return memochat::moments::gateway_runtime::modules::DefaultListenPort();
}

bool ShouldInitAws()
{
    return memochat::moments::gateway_runtime::modules::ShouldInitAws();
}
} // namespace memochat::tests::moments::gateway_runtime
