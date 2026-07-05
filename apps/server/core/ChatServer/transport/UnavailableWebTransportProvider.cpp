#include "UnavailableWebTransportProvider.hpp"

#include "logging/Logger.hpp"

namespace memochat::chatserver
{

bool UnavailableWebTransportProvider::Start(const WebTransportListenOptions& options,
                                            WebTransportProviderSessionHooks hooks,
                                            std::string* error)
{
    (void) hooks;
    if (error)
    {
        *error = "webtransport_h3_library_not_configured";
    }
    memolog::LogWarn(
        "webtransport.provider.unavailable",
        "WebTransport provider is unavailable until an HTTP/3 WebTransport server library is configured",
        {{"provider", providerName()}, {"host", options.host}, {"port", options.port}, {"path", options.path}});
    return false;
}

void UnavailableWebTransportProvider::Stop()
{
}

bool UnavailableWebTransportProvider::isRunning() const
{
    return false;
}

std::string UnavailableWebTransportProvider::providerName() const
{
    return "unavailable";
}

} // namespace memochat::chatserver
