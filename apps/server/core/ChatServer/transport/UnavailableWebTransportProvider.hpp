#pragma once

#include "IWebTransportProvider.hpp"

namespace memochat::chatserver
{

class UnavailableWebTransportProvider final : public IWebTransportProvider
{
public:
    bool Start(const WebTransportListenOptions& options,
               WebTransportProviderSessionHooks hooks,
               std::string* error) override;
    void Stop() override;
    bool isRunning() const override;
    std::string providerName() const override;
};

} // namespace memochat::chatserver
