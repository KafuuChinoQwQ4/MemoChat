#include "RelationServiceConfig.hpp"

#include "ConfigMgr.hpp"

import memochat.chat.config_algorithms;

std::string RelationServiceConfig::RelationServiceBackend() const
{
    auto backend = ConfigMgr::Inst().GetValue("RelationService", "Backend");
    if (backend.empty())
    {
        return "inprocess";
    }
    memochat::chat::config::modules::LowerAsciiInPlace(backend.data(), backend.size());
    return backend;
}

std::string RelationServiceConfig::RelationServiceEndpoint() const
{
    return ConfigMgr::Inst().GetValue("RelationService", "Endpoint");
}
