#include "MessageServiceConfig.hpp"

#include "ConfigMgr.hpp"

import memochat.chat.config_algorithms;

std::string MessageServiceConfig::MessageServiceBackend() const
{
    auto backend = ConfigMgr::Inst().GetValue("MessageService", "Backend");
    if (backend.empty())
    {
        return "inprocess";
    }
    memochat::chat::config::modules::LowerAsciiInPlace(backend.data(), backend.size());
    return backend;
}

std::string MessageServiceConfig::MessageServiceEndpoint() const
{
    return ConfigMgr::Inst().GetValue("MessageService", "Endpoint");
}
