#include "RelationServiceConfig.h"

#include "ConfigMgr.h"

#include <algorithm>
#include <cctype>

std::string RelationServiceConfig::RelationServiceBackend() const
{
    auto backend = ConfigMgr::Inst().GetValue("RelationService", "Backend");
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

std::string RelationServiceConfig::RelationServiceEndpoint() const
{
    return ConfigMgr::Inst().GetValue("RelationService", "Endpoint");
}
