#include "OnlineRouteSessionSupport.hpp"

#include "CSession.hpp"

std::string ExtractOnlineRouteSessionId(const std::shared_ptr<CSession>& session)
{
    if (!session)
    {
        return std::string();
    }
    return session->GetSessionId();
}
