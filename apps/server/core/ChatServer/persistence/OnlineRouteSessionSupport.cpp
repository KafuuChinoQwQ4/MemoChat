#include "OnlineRouteSessionSupport.hpp"

#include "IChatSession.hpp"

std::string ExtractOnlineRouteSessionId(const std::shared_ptr<IChatSession>& session)
{
    if (!session)
    {
        return std::string();
    }
    return session->sessionId();
}
