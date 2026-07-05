#pragma once

#include "IWebTransportProvider.hpp"

#include <memory>

namespace memochat::chatserver
{

std::unique_ptr<IWebTransportProvider> CreateDefaultWebTransportProvider();

} // namespace memochat::chatserver
