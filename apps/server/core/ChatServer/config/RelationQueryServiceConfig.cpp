#include "RelationQueryServiceConfig.h"

#include "ConfigMgr.h"

#include <algorithm>
#include <cctype>

std::string RelationQueryServiceConfig::RelationQueryServiceBackend() const
{
    auto backend = ConfigMgr::Inst().GetValue("RelationQueryService", "Backend");
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

std::string RelationQueryServiceConfig::RelationQueryServiceEndpoint() const
{
    return ConfigMgr::Inst().GetValue("RelationQueryService", "Endpoint");
}
