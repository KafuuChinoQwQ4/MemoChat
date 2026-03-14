#include "ChatRuntime.h"

#include "ConfigMgr.h"

#include <algorithm>

namespace {
std::string NormalizeLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<char>(ch - 'A' + 'a');
        }
        return static_cast<char>(ch);
    });
    return value;
}

bool ParseBoolFlag(const std::string& raw, bool default_value = false)
{
    if (raw.empty()) {
        return default_value;
    }
    const auto value = NormalizeLower(raw);
    return value == "1" || value == "true" || value == "yes" || value == "on";
}
}

namespace memochat::chatruntime {

ChatNodeRole CurrentRole()
{
    auto role = NormalizeLower(ConfigMgr::Inst().GetValue("Runtime", "Role"));
    if (role == "worker") {
        return ChatNodeRole::Worker;
    }
    if (role == "hybrid") {
        return ChatNodeRole::Hybrid;
    }
    return ChatNodeRole::Ingress;
}

bool IsIngressEnabled()
{
    const auto role = CurrentRole();
    return role == ChatNodeRole::Ingress || role == ChatNodeRole::Hybrid;
}

bool IsWorkerEnabled()
{
    const auto role = CurrentRole();
    return role == ChatNodeRole::Worker || role == ChatNodeRole::Hybrid;
}

bool FeatureEnabled(const std::string& name)
{
    return ParseBoolFlag(ConfigMgr::Inst().GetValue("FeatureFlags", name));
}

std::string SelfServerName()
{
    return ConfigMgr::Inst()["SelfServer"]["Name"];
}

std::string QueueKeyForTopic(const std::string& topic)
{
    return std::string("kafka_queue:") + topic;
}

std::string TopicPrivate()
{
    return "chat.private.v1";
}

std::string TopicGroup()
{
    return "chat.group.v1";
}

}
