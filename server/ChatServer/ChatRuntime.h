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
std::string AsyncEventBusBackend();
std::string TaskBusBackend();
std::string SelfServerName();
std::string QueueKeyForTopic(const std::string& topic);
std::string TopicPrivate();
std::string TopicGroup();
std::string TopicDialogSync();
std::string TopicRelationState();
std::string TopicUserProfileChanged();
std::string TopicAuditLogin();
std::string TopicPrivateDlq();
std::string TopicGroupDlq();
std::string TaskRoutingDeliveryRetry();
std::string TaskRoutingOfflineNotify();
std::string TaskRoutingRelationNotify();
std::string TaskRoutingOutboxRepair();
int TaskRetryDelayMs();
int TaskMaxRetries();

}
