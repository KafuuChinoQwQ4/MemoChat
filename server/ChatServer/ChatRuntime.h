#pragma once

#include <string>

namespace memochat::chatruntime {

enum class ChatNodeRole {
    Ingress,
    Worker,
    Hybrid
};

ChatNodeRole CurrentRole();
bool IsIngressEnabled();
bool IsWorkerEnabled();
bool FeatureEnabled(const std::string& name);
std::string SelfServerName();
std::string QueueKeyForTopic(const std::string& topic);
std::string TopicPrivate();
std::string TopicGroup();

}
