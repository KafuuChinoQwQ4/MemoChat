#include "MessageDeliveryService.h"

#include "ChatGrpcClient.h"
#include "ChatRuntime.h"
#include "ConfigMgr.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "logging/Logger.h"
#include "cluster/ChatClusterDiscovery.h"

#include <algorithm>
#include <json/json.h>
#include <unordered_map>
#include <unordered_set>

namespace {
enum class OnlineRouteKind {
    Offline,
    Local,
    Remote,
    Stale
};

struct OnlineRouteDecision {
    OnlineRouteKind kind = OnlineRouteKind::Offline;
    std::shared_ptr<CSession> session;
    std::string redis_server;
    bool local_session_found = false;
};

std::string TrimCopyDelivery(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string ServerOnlineUsersKeyDelivery(const std::string& server_name) {
    return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

std::vector<std::string> KnownChatServerNamesDelivery() {
    std::vector<std::string> servers;
    auto& cfg = ConfigMgr::Inst();
    static const auto cluster = memochat::cluster::LoadChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key) {
            return cfg.GetValue(section, key);
        },
        TrimCopyDelivery(cfg["SelfServer"]["Name"]));
    for (const auto& node : cluster.enabledNodes()) {
        servers.push_back(node.name);
    }
    return servers;
}

void RepairOnlineRouteStateDelivery(int uid, const std::shared_ptr<CSession>& session, const std::string& server_name) {
    if (uid <= 0 || !session || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
    RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
    RedisMgr::GetInstance()->SAdd(ServerOnlineUsersKeyDelivery(server_name), uid_str);
}

std::string ResolveServerFromOnlineSetsDelivery(const std::string& uid_str) {
    if (uid_str.empty()) {
        return std::string();
    }

    for (const auto& server_name : KnownChatServerNamesDelivery()) {
        std::vector<std::string> online_uids;
        RedisMgr::GetInstance()->SMembers(ServerOnlineUsersKeyDelivery(server_name), online_uids);
        if (std::find(online_uids.begin(), online_uids.end(), uid_str) != online_uids.end()) {
            return server_name;
        }
    }
    return std::string();
}

void ClearTrackedOnlineRouteDelivery(int uid, const std::string& server_name) {
    if (uid <= 0 || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    RedisMgr::GetInstance()->SRem(ServerOnlineUsersKeyDelivery(server_name), uid_str);
}

OnlineRouteDecision ResolveOnlineRouteDelivery(int uid) {
    OnlineRouteDecision route;
    if (uid <= 0) {
        return route;
    }

    const auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];
    auto session = UserMgr::GetInstance()->GetSession(uid);
    if (session) {
        route.kind = OnlineRouteKind::Local;
        route.session = session;
        route.redis_server = self_name;
        route.local_session_found = true;
        RepairOnlineRouteStateDelivery(uid, session, self_name);
        return route;
    }

    const auto uid_str = std::to_string(uid);
    std::string redis_server;
    if (!RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, redis_server) || redis_server.empty()) {
        redis_server = ResolveServerFromOnlineSetsDelivery(uid_str);
        if (redis_server.empty()) {
            return route;
        }
    }
    route.redis_server = redis_server;
    if (redis_server != self_name) {
        route.kind = OnlineRouteKind::Remote;
        return route;
    }

    std::string reloaded_server;
    if (RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, reloaded_server) && !reloaded_server.empty()) {
        route.redis_server = reloaded_server;
        if (reloaded_server != self_name) {
            route.kind = OnlineRouteKind::Remote;
            return route;
        }
    }

    route.kind = OnlineRouteKind::Stale;
    ClearTrackedOnlineRouteDelivery(uid, self_name);
    return route;
}

enum class DeliveryAttemptResult {
    Delivered,
    Offline,
    RetryableFailure
};

Json::Value BuildDeliveryTaskPayload(int recipient_uid, short msgid, const Json::Value& payload, int exclude_uid, const char* reason)
{
    Json::Value task(Json::objectValue);
    task["recipient_uid"] = recipient_uid;
    task["msgid"] = msgid;
    task["exclude_uid"] = exclude_uid;
    task["reason"] = reason;
    task["payload"] = payload;
    return task;
}
}

MessageDeliveryService::MessageDeliveryService(PublishTaskFn publish_task)
    : _publish_task(std::move(publish_task)) {
}

void MessageDeliveryService::PushPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid)
{
    TryPushPayload(recipients, msgid, payload, exclude_uid, true);
}

bool MessageDeliveryService::TryPushPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid, bool enqueue_on_failure)
{
    if (recipients.empty()) {
        return true;
    }

    std::unordered_set<int> uniq;
    for (int uid : recipients) {
        if (uid <= 0 || uid == exclude_uid) {
            continue;
        }
        uniq.insert(uid);
    }
    if (uniq.empty()) {
        return true;
    }

    const std::string payload_str = payload.toStyledString();
    std::unordered_map<std::string, std::vector<int>> remote_server_uids;
    bool all_delivered = true;

    if (msgid == ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ) {
        for (int uid : uniq) {
            auto route = ResolveOnlineRouteDelivery(uid);
            if (route.kind == OnlineRouteKind::Local && route.session) {
                route.session->Send(payload_str, msgid);
                continue;
            }

            std::string target_server;
            if (route.kind == OnlineRouteKind::Remote) {
                target_server = route.redis_server;
            }
            if (target_server.empty()) {
                target_server = ResolveServerFromOnlineSetsDelivery(std::to_string(uid));
            }
            if (target_server.empty()) {
                all_delivered = false;
                if (enqueue_on_failure && _publish_task) {
                    std::string error;
                    _publish_task("offline_notify",
                        memochat::chatruntime::TaskRoutingOfflineNotify(),
                        BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "offline"),
                        memochat::chatruntime::TaskRetryDelayMs(),
                        memochat::chatruntime::TaskMaxRetries(),
                        &error);
                }
                continue;
            }

            GroupMemberBatchReq req;
            req.set_tcp_msgid(msgid);
            req.set_payload_json(payload_str);
            req.add_touids(uid);
            auto rsp = ChatGrpcClient::GetInstance()->NotifyGroupMemberBatch(target_server, req);
            if (rsp.error() == ErrorCodes::Success && rsp.delivered() > 0) {
                RedisMgr::GetInstance()->Set(USERIPPREFIX + std::to_string(uid), target_server);
                continue;
            }

            const auto fallback_server = ResolveServerFromOnlineSetsDelivery(std::to_string(uid));
            if (fallback_server.empty() || fallback_server == target_server) {
                continue;
            }

            auto fallback_rsp = ChatGrpcClient::GetInstance()->NotifyGroupMemberBatch(fallback_server, req);
            if (fallback_rsp.error() == ErrorCodes::Success && fallback_rsp.delivered() > 0) {
                RedisMgr::GetInstance()->Set(USERIPPREFIX + std::to_string(uid), fallback_server);
                continue;
            }

            all_delivered = false;
            if (enqueue_on_failure && _publish_task) {
                std::string error;
                _publish_task("message_delivery_retry",
                    memochat::chatruntime::TaskRoutingDeliveryRetry(),
                    BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "rpc_failed"),
                    memochat::chatruntime::TaskRetryDelayMs(),
                    memochat::chatruntime::TaskMaxRetries(),
                    &error);
            }
        }
        return all_delivered;
    }

    for (int uid : uniq) {
        const auto route = ResolveOnlineRouteDelivery(uid);
        if (route.kind == OnlineRouteKind::Local && route.session) {
            route.session->Send(payload_str, msgid);
            continue;
        }
        if (route.kind == OnlineRouteKind::Remote) {
            remote_server_uids[route.redis_server].push_back(uid);
            continue;
        }
        all_delivered = false;
        if (enqueue_on_failure && _publish_task) {
            std::string error;
            _publish_task("offline_notify",
                memochat::chatruntime::TaskRoutingOfflineNotify(),
                BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "offline"),
                memochat::chatruntime::TaskRetryDelayMs(),
                memochat::chatruntime::TaskMaxRetries(),
                &error);
        }
    }

    for (auto& entry : remote_server_uids) {
        auto& server_name = entry.first;
        auto& uids = entry.second;
        if (uids.empty()) {
            continue;
        }

        if (msgid == ID_NOTIFY_GROUP_CHAT_MSG_REQ) {
            GroupMessageNotifyReq req;
            req.set_tcp_msgid(msgid);
            req.set_payload_json(payload_str);
            for (int uid : uids) {
                req.add_touids(uid);
            }
            const auto rsp = ChatGrpcClient::GetInstance()->NotifyGroupMessage(server_name, req);
            if (rsp.error() != ErrorCodes::Success || rsp.delivered() <= 0) {
                all_delivered = false;
                if (enqueue_on_failure && _publish_task) {
                    for (int uid : uids) {
                        std::string error;
                        _publish_task("message_delivery_retry",
                            memochat::chatruntime::TaskRoutingDeliveryRetry(),
                            BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "rpc_failed"),
                            memochat::chatruntime::TaskRetryDelayMs(),
                            memochat::chatruntime::TaskMaxRetries(),
                            &error);
                    }
                }
            }
            continue;
        }

        GroupEventNotifyReq req;
        req.set_tcp_msgid(msgid);
        req.set_payload_json(payload_str);
        for (int uid : uids) {
            req.add_touids(uid);
        }
        const auto rsp = ChatGrpcClient::GetInstance()->NotifyGroupEvent(server_name, req);
        if (rsp.error() != ErrorCodes::Success || rsp.delivered() <= 0) {
            all_delivered = false;
            if (enqueue_on_failure && _publish_task) {
                for (int uid : uids) {
                    std::string error;
                    _publish_task("message_delivery_retry",
                        memochat::chatruntime::TaskRoutingDeliveryRetry(),
                        BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "rpc_failed"),
                        memochat::chatruntime::TaskRetryDelayMs(),
                        memochat::chatruntime::TaskMaxRetries(),
                        &error);
                }
            }
        }
    }

    return all_delivered;
}
