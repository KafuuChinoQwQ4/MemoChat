#include "WebTransportProviderFactory.hpp"

#include "UnavailableWebTransportProvider.hpp"

#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER
#include "LibwebsocketsWebTransportProvider.hpp"
#endif

namespace memochat::chatserver
{

std::unique_ptr<IWebTransportProvider> CreateDefaultWebTransportProvider()
{
#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER
    return std::make_unique<LibwebsocketsWebTransportProvider>();
#else
    return std::make_unique<UnavailableWebTransportProvider>();
#endif
}

} // namespace memochat::chatserver
