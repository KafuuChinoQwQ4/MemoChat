#include "ChatSessionConfig.hpp"

#include "ConfigMgr.hpp"
#include "auth/AuthSecret.hpp"
#include "logging/Logger.hpp"

#include <string>

import memochat.chat.config_algorithms;

std::string ChatSessionConfig::ChatAuthSecret() const
{
    auto value = ConfigMgr::Inst().GetValue("ChatAuth", "HmacSecret");
    if (value.empty())
    {
        value = "memochat-dev-chat-secret";
    }
    if (memochat::auth::IsWellKnownDevHmacSecret(value))
    {
        static bool warned_dev_secret = false;
        if (!warned_dev_secret)
        {
            warned_dev_secret = true;
            memolog::LogWarn("chat.config.dev_hmac_secret",
                             "ChatAuth HmacSecret is the well-known dev value; set env "
                             "MEMOCHAT_CHATAUTH_HMACSECRET in production");
        }
    }
    return value;
}

bool ChatSessionConfig::FeatureFlagDefaultTrue(const std::string& name) const
{
    auto raw = ConfigMgr::Inst().GetValue("FeatureFlags", name);
    return memochat::chat::config::modules::FeatureFlagDefaultTrue(raw.data(), raw.size());
}

int ChatSessionConfig::HeartbeatRouteRefreshSec() const
{
    return ConfigInt("LogicSystem", "HeartbeatRouteRefreshSec", 15, 1, 300);
}

int ChatSessionConfig::LoginOfflinePushDelayMs() const
{
    return ConfigInt("LogicSystem", "LoginOfflinePushDelayMs", 0, 0, 60000);
}

std::string ChatSessionConfig::SelfServerName() const
{
    return ConfigMgr::Inst()["SelfServer"]["Name"];
}

int ChatSessionConfig::ConfigInt(const std::string& section,
                                 const std::string& key,
                                 int default_value,
                                 int min_value,
                                 int max_value) const
{
    const auto raw = ConfigMgr::Inst().GetValue(section, key);
    if (raw.empty())
    {
        return default_value;
    }
    try
    {
        return memochat::chat::config::modules::ClampInt(std::stoi(raw), min_value, max_value);
    }
    catch (...)
    {
        return default_value;
    }
}
