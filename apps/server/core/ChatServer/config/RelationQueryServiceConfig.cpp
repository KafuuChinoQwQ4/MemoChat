#include "RelationQueryServiceConfig.hpp"

#include "ConfigMgr.hpp"

import memochat.chat.config_algorithms;

std::string RelationQueryServiceConfig::RelationQueryServiceBackend() const
{
    auto backend = ConfigMgr::Inst().GetValue("RelationQueryService", "Backend");
    if (backend.empty())
    {
        return "inprocess";
    }
    memochat::chat::config::modules::LowerAsciiInPlace(backend.data(), backend.size());
    return backend;
}

std::string RelationQueryServiceConfig::RelationQueryServiceEndpoint() const
{
    return ConfigMgr::Inst().GetValue("RelationQueryService", "Endpoint");
}
