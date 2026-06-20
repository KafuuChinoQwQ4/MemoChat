#include "MessageDeliveryService.h"

#include "ChatGrpcClient.h"
#include "ChatRuntime.h"
#include "CSession.h"
#include "delivery/MessageDeliveryTaskPayload.h"
#include "MessageDeliveryPayload.h"
#include "ports/OnlineRouteResolver.h"
#include "logging/Logger.h"

#include "json/GlazeCompat.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
using memochat::chat::routing::OnlineRouteDecision;
using memochat::chat::routing::OnlineRouteKind;
using memochat::chat::routing::ResolveOnlineRoute;

enum class DeliveryAttemptResult
{
    Delivered,
    Offline,
    RetryableFailure
};

memochat::json::JsonValue BuildDeliveryTaskPayload(int recipient_uid,
                                                   short msgid,
                                                   const memochat::json::JsonValue& payload,
                                                   int exclude_uid,
                                                   const char* reason)
{
    return memochat::chat::delivery::BuildDeliveryTaskPayloadJson(recipient_uid,
                                                                  msgid,
                                                                  payload,
                                                                  exclude_uid,
                                                                  reason == nullptr ? "" : reason);
}
} // namespace

MessageDeliveryService::MessageDeliveryService(IDeliveryTaskPublisher* task_publisher,
                                               ISessionRegistry* session_registry,
                                               IOnlineRouteStore* online_route_store)
    : _task_publisher(task_publisher)
    , _session_registry(session_registry)
    , _online_route_store(online_route_store)
{
}

void MessageDeliveryService::SetTaskPublisher(IDeliveryTaskPublisher* task_publisher)
{
    _task_publisher = task_publisher;
}

void MessageDeliveryService::SetRouteDependencies(ISessionRegistry* session_registry,
                                                  IOnlineRouteStore* online_route_store)
{
    _session_registry = session_registry;
    _online_route_store = online_route_store;
}

void MessageDeliveryService::PushPayload(const std::vector<int>& recipients,
                                         short msgid,
                                         const memochat::json::JsonValue& payload,
                                         int exclude_uid)
{
    TryPushPayload(recipients, msgid, payload, exclude_uid, true);
}

bool MessageDeliveryService::TryPushPayload(const std::vector<int>& recipients,
                                            short msgid,
                                            const memochat::json::JsonValue& payload,
                                            int exclude_uid,
                                            bool enqueue_on_failure)
{
    if (recipients.empty())
    {
        return true;
    }

    std::unordered_set<int> uniq;
    for (int uid : recipients)
    {
        if (uid <= 0 || uid == exclude_uid)
        {
            continue;
        }
        uniq.insert(uid);
    }
    if (uniq.empty())
    {
        return true;
    }

    const std::string payload_str = memochat::chat::delivery::SerializeDeliveryPayloadForWire(payload);
    std::unordered_map<std::string, std::vector<int>> remote_server_uids;
    bool all_delivered = true;

    if (msgid == ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ)
    {
        for (int uid : uniq)
        {
            auto route = ResolveOnlineRoute(uid, _session_registry, _online_route_store);
            if (route.kind == OnlineRouteKind::Local && route.session)
            {
                route.session->Send(payload_str, msgid);
                continue;
            }

            std::string target_server;
            if (route.kind == OnlineRouteKind::Remote)
            {
                target_server = route.redis_server;
            }
            if (target_server.empty())
            {
                target_server =
                    _online_route_store ? _online_route_store->ResolveServerFromOnlineSets(uid) : std::string();
            }
            if (target_server.empty())
            {
                all_delivered = false;
                if (enqueue_on_failure && _task_publisher)
                {
                    std::string error;
                    _task_publisher->PublishDeliveryTask(
                        "offline_notify",
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
            if (rsp.error() == ErrorCodes::Success && rsp.delivered() > 0)
            {
                if (_online_route_store)
                {
                    _online_route_store->SetUserServer(uid, target_server);
                }
                continue;
            }

            const auto fallback_server =
                _online_route_store ? _online_route_store->ResolveServerFromOnlineSets(uid) : std::string();
            if (fallback_server.empty() || fallback_server == target_server)
            {
                continue;
            }

            auto fallback_rsp = ChatGrpcClient::GetInstance()->NotifyGroupMemberBatch(fallback_server, req);
            if (fallback_rsp.error() == ErrorCodes::Success && fallback_rsp.delivered() > 0)
            {
                if (_online_route_store)
                {
                    _online_route_store->SetUserServer(uid, fallback_server);
                }
                continue;
            }

            all_delivered = false;
            if (enqueue_on_failure && _task_publisher)
            {
                std::string error;
                _task_publisher->PublishDeliveryTask(
                    "message_delivery_retry",
                    memochat::chatruntime::TaskRoutingDeliveryRetry(),
                    BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "rpc_failed"),
                    memochat::chatruntime::TaskRetryDelayMs(),
                    memochat::chatruntime::TaskMaxRetries(),
                    &error);
            }
        }
        return all_delivered;
    }

    for (int uid : uniq)
    {
        const auto route = ResolveOnlineRoute(uid, _session_registry, _online_route_store);
        if (route.kind == OnlineRouteKind::Local && route.session)
        {
            route.session->Send(payload_str, msgid);
            continue;
        }
        if (route.kind == OnlineRouteKind::Remote)
        {
            remote_server_uids[route.redis_server].push_back(uid);
            continue;
        }
        all_delivered = false;
        if (enqueue_on_failure && _task_publisher)
        {
            std::string error;
            _task_publisher->PublishDeliveryTask("offline_notify",
                                                 memochat::chatruntime::TaskRoutingOfflineNotify(),
                                                 BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "offline"),
                                                 memochat::chatruntime::TaskRetryDelayMs(),
                                                 memochat::chatruntime::TaskMaxRetries(),
                                                 &error);
        }
    }

    for (auto& entry : remote_server_uids)
    {
        auto& server_name = entry.first;
        auto& uids = entry.second;
        if (uids.empty())
        {
            continue;
        }

        if (msgid == ID_NOTIFY_GROUP_CHAT_MSG_REQ)
        {
            GroupMessageNotifyReq req;
            req.set_tcp_msgid(msgid);
            req.set_payload_json(payload_str);
            for (int uid : uids)
            {
                req.add_touids(uid);
            }
            const auto rsp = ChatGrpcClient::GetInstance()->NotifyGroupMessage(server_name, req);
            if (rsp.error() != ErrorCodes::Success || rsp.delivered() <= 0)
            {
                all_delivered = false;
                if (enqueue_on_failure && _task_publisher)
                {
                    for (int uid : uids)
                    {
                        std::string error;
                        _task_publisher->PublishDeliveryTask(
                            "message_delivery_retry",
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

        if (msgid == ID_NOTIFY_TEXT_CHAT_MSG_REQ)
        {
            for (int uid : uids)
            {
                message::TextChatMsgReq req;
                if (!memochat::chat::delivery::BuildPrivateTextNotifyRequest(uid, payload, &req))
                {
                    all_delivered = false;
                    if (enqueue_on_failure && _task_publisher)
                    {
                        std::string error;
                        _task_publisher->PublishDeliveryTask(
                            "message_delivery_retry",
                            memochat::chatruntime::TaskRoutingDeliveryRetry(),
                            BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "invalid_private_notify"),
                            memochat::chatruntime::TaskRetryDelayMs(),
                            memochat::chatruntime::TaskMaxRetries(),
                            &error);
                    }
                    continue;
                }

                const auto rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(server_name, req, payload);
                if (rsp.error() == ErrorCodes::Success)
                {
                    continue;
                }

                all_delivered = false;
                if (enqueue_on_failure && _task_publisher)
                {
                    std::string error;
                    _task_publisher->PublishDeliveryTask(
                        "message_delivery_retry",
                        memochat::chatruntime::TaskRoutingDeliveryRetry(),
                        BuildDeliveryTaskPayload(uid, msgid, payload, exclude_uid, "rpc_failed"),
                        memochat::chatruntime::TaskRetryDelayMs(),
                        memochat::chatruntime::TaskMaxRetries(),
                        &error);
                }
            }
            continue;
        }

        GroupEventNotifyReq req;
        req.set_tcp_msgid(msgid);
        req.set_payload_json(payload_str);
        for (int uid : uids)
        {
            req.add_touids(uid);
        }
        const auto rsp = ChatGrpcClient::GetInstance()->NotifyGroupEvent(server_name, req);
        if (rsp.error() != ErrorCodes::Success || rsp.delivered() <= 0)
        {
            all_delivered = false;
            if (enqueue_on_failure && _task_publisher)
            {
                for (int uid : uids)
                {
                    std::string error;
                    _task_publisher->PublishDeliveryTask(
                        "message_delivery_retry",
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
