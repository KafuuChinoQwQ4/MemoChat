#pragma once

#include "IWebTransportSession.hpp"

#include <functional>
#include <memory>
#include <string>

namespace memochat::chatserver
{

struct WebTransportListenOptions
{
    std::string host;
    std::string port;
    std::string path;
    std::string cert_file;
    std::string private_key_file;
};

struct WebTransportProviderSessionHooks
{
    using OpenSessionCallback =
        std::function<std::shared_ptr<IWebTransportSession>(WebTransportSendFrameCallback, WebTransportCloseCallback)>;

    OpenSessionCallback open_session;
};

class IWebTransportProvider
{
public:
    virtual ~IWebTransportProvider() = default;

    virtual bool
    Start(const WebTransportListenOptions& options, WebTransportProviderSessionHooks hooks, std::string* error) = 0;
    virtual void Stop() = 0;
    virtual bool isRunning() const = 0;
    virtual std::string providerName() const = 0;
};

} // namespace memochat::chatserver
