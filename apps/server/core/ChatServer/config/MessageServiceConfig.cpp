#include "MessageServiceConfig.h"

#include "ConfigMgr.h"

#include <algorithm>
#include <cctype>

std::string MessageServiceConfig::MessageServiceBackend() const
{
    auto backend = ConfigMgr::Inst().GetValue("MessageService", "Backend");
    if (backend.empty())
    {
        return "inprocess";
    }
    std::transform(backend.begin(),
                   backend.end(),
                   backend.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return backend;
}

std::string MessageServiceConfig::MessageServiceEndpoint() const
{
    return ConfigMgr::Inst().GetValue("MessageService", "Endpoint");
}
