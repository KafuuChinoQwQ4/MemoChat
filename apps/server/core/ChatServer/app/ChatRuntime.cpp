#include "ChatRuntime.hpp"

#include "ConfigMgr.hpp"

#include <algorithm>
#include <charconv>

import memochat.chat.runtime_algorithms;

namespace
{
std::string NormalizeLower(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch)
                   {
                       return memochat::chat::runtime::modules::ToLowerAscii(ch);
                   });
    return value;
}

bool ParseBoolFlag(const std::string& raw, bool default_value = false)
{
    return memochat::chat::runtime::modules::ParseBoolFlagOr(raw.data(), raw.size(), default_value);
}

int ParsePositiveIntOr(const std::string& raw, int fallback, int minimum)
{
    int value = 0;
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != raw.data() + raw.size())
    {
        return fallback;
    }
    return memochat::chat::runtime::modules::AtLeast(value, minimum);
}
} // namespace

namespace memochat::chatruntime
{

ChatNodeRole CurrentRole()
{
    auto role = NormalizeLower(ConfigMgr::Inst().GetValue("Runtime", "Role"));
    if (role == "ingress")
    {
        return ChatNodeRole::Ingress;
    }
    if (role == "worker")
    {
        return ChatNodeRole::Worker;
    }
    if (role == "hybrid")
    {
        return ChatNodeRole::Hybrid;
    }
    return ChatNodeRole::Hybrid;
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

std::string AsyncEventBusBackend()
{
    const auto backend = NormalizeLower(ConfigMgr::Inst().GetValue("Runtime", "AsyncEventBus"));
    if (backend.empty())
    {
        return "redis";
    }
    return backend;
}

std::string TaskBusBackend()
{
    const auto backend = NormalizeLower(ConfigMgr::Inst().GetValue("Runtime", "TaskBus"));
    if (backend.empty())
    {
        return "inline";
    }
    return backend;
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

std::string TopicDialogSync()
{
    return "dialog.sync.v1";
}

std::string TopicRelationState()
{
    return "relation.state.v1";
}

std::string TopicUserProfileChanged()
{
    return "user.profile.changed.v1";
}

std::string TopicAuditLogin()
{
    return "audit.login.v1";
}

std::string TopicPrivateDlq()
{
    return "chat.private.dlq.v1";
}

std::string TopicGroupDlq()
{
    return "chat.group.dlq.v1";
}

std::string TaskRoutingDeliveryRetry()
{
    return "chat.delivery.retry";
}

std::string TaskRoutingOfflineNotify()
{
    return "chat.notify.offline";
}

std::string TaskRoutingRelationNotify()
{
    return "relation.notify";
}

std::string TaskRoutingOutboxRepair()
{
    return "chat.outbox.relay.retry";
}

int TaskRetryDelayMs()
{
    const auto raw = ConfigMgr::Inst().GetValue("RabbitMQ", "RetryDelayMs");
    if (raw.empty())
    {
        return 5000;
    }
    return ParsePositiveIntOr(raw, 5000, 100);
}

int TaskMaxRetries()
{
    const auto raw = ConfigMgr::Inst().GetValue("RabbitMQ", "MaxRetries");
    if (raw.empty())
    {
        return 5;
    }
    return ParsePositiveIntOr(raw, 5, 1);
}

} // namespace memochat::chatruntime
