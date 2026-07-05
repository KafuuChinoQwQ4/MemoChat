#pragma once

#include "IChatSession.hpp"

#include <memory>

class IChatSessionHost;

namespace memochat::chatserver
{

void CleanupExceptionSession(const std::shared_ptr<IChatSession>& session, IChatSessionHost* host = nullptr);

} // namespace memochat::chatserver
