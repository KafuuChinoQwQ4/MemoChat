#include "PrivateMessageService.h"

#include "ChatGrpcClient.h"
#include "ChatHistoryCommandDtos.h"
#include "ChatHistoryOutputDtos.h"
#include "ChatMessageCommandDtos.h"
#include "ChatRuntime.h"
#include "CSession.h"
#include "MessageServiceUtil.h"
#include "ports/OnlineRouteResolver.h"
#include "logging/Logger.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include "json/GlazeCompat.h"
#include <vector>

namespace
{
using memochat::chat::message::JsonToWireString;
using memochat::chat::message::NowMs;
namespace ChatHistoryCommand = memochat::chat::history;
namespace ChatHistoryOutput = memochat::chat::history::output;
namespace ChatMessageCommand = memochat::chat::command;
using memochat::chat::routing::OnlineRouteDecision;
using memochat::chat::routing::OnlineRouteKind;
using memochat::chat::routing::OnlineRouteResultName;
using memochat::chat::routing::ResolveOnlineRoute;

std::string JsonToCompactStringLocal(const memochat::json::JsonValue& value)
{
    return memochat::chat::message::JsonToWireString(value);
}

bool ParseJsonObjectLocal(const std::string& payload, memochat::json::JsonValue& root)
{
    memochat::json::JsonCharReaderBuilder builder;
    std::unique_ptr<memochat::json::JsonCharReader> reader(builder.newCharReader());
    std::string errors;
    return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.is_object();
}

void LogPrivateRouteLocal(const std::string& event,
                          int from_uid,
                          int to_uid,
                          const std::string& msg_id,
                          const OnlineRouteDecision& route,
                          const std::string& grpc_status,
                          bool notify_delivered)
{
    const auto fields =
        std::map<std::string, std::string>{{"from_uid", std::to_string(from_uid)},
                                           {"to_uid", std::to_string(to_uid)},
                                           {"msg_id", msg_id},
                                           {"redis_server", route.redis_server},
                                           {"route_result", OnlineRouteResultName(route.kind)},
                                           {"local_session_found", route.local_session_found ? "true" : "false"},
                                           {"grpc_status", grpc_status},
                                           {"notify_delivered", notify_delivered ? "true" : "false"}};
    if (route.kind == OnlineRouteKind::Stale || !notify_delivered)
    {
        memolog::LogWarn(event, "private message notify not delivered", fields);
        return;
    }
    memolog::LogInfo(event, "private message notify delivered", fields);
}

bool KafkaBackendEnabledLocal()
{
    return memochat::chatruntime::AsyncEventBusBackend() == "kafka";
}

MessageCommandRequest
BuildMessageCommandRequestLocal(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data)
{
    MessageCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    request.server_name = memochat::chatruntime::SelfServerName();
    if (session)
    {
        request.session_uid = session->GetUserId();
        request.session_id = session->GetSessionId();
    }
    return request;
}

void SendMessageCommandResultLocal(const std::shared_ptr<CSession>& session, const MessageCommandResult& result)
{
    if (!session || result.response_msg_id == 0)
    {
        return;
    }
    session->Send(result.payload_json, result.response_msg_id);
}
} // namespace

PrivateMessageService::PrivateMessageService(ISessionRegistry* session_registry,
                                             IOnlineRouteStore* online_route_store,
                                             IMessageRepository* message_repository,
                                             IDeliveryGateway* delivery_gateway,
                                             IEventPublisher* event_publisher)
    : _session_registry(session_registry)
    , _online_route_store(online_route_store)
    , _message_repository(message_repository)
    , _delivery_gateway(delivery_gateway)
    , _event_publisher(event_publisher)
{
}

MessageCommandResult PrivateMessageService::TextChatMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonValue root;
    ParseJsonObjectLocal(request.payload_json, root);

    const auto uid = root["fromuid"].asInt();
    const auto touid = root["touid"].asInt();
    const memochat::json::JsonValue arrays = root["text_array"];
    const bool kafka_backend = KafkaBackendEnabledLocal();
    const bool kafka_primary = kafka_backend && memochat::chatruntime::FeatureEnabled("chat_private_kafka_primary");
    const bool kafka_shadow =
        kafka_backend && !kafka_primary && memochat::chatruntime::FeatureEnabled("chat_private_kafka_shadow");

    ChatMessageCommand::ChatPrivateSendResponseDto rtdto;
    rtdto.error = ErrorCodes::Success;
    rtdto.fromuid = uid;
    rtdto.touid = touid;

    const auto result = [&rtdto]()
    {
        return MessageCommandResult{ID_TEXT_CHAT_MSG_RSP,
                                    JsonToCompactStringLocal(ChatMessageCommand::ToJsonValue(rtdto))};
    };

    if (uid <= 0 || touid <= 0 || !arrays.is_array() || arrays.empty())
    {
        rtdto.error = ErrorCodes::Error_Json;
        return result();
    }

    memochat::json::JsonValue normalized(memochat::json::array_t{});
    std::vector<PrivateMessageInfo> pending_messages;
    const int conv_uid_min = std::min(uid, touid);
    const int conv_uid_max = std::max(uid, touid);
    std::string first_msg_id;

    for (const auto& txt_obj : arrays)
    {
        PrivateMessageInfo msg;
        msg.msg_id = txt_obj["msgid"].asString();
        msg.content = txt_obj["content"].asString();
        msg.conv_uid_min = conv_uid_min;
        msg.conv_uid_max = conv_uid_max;
        msg.from_uid = uid;
        msg.to_uid = touid;
        msg.reply_to_server_msg_id = txt_obj.get("reply_to_server_msg_id", 0).asInt64();
        msg.edited_at_ms = txt_obj.get("edited_at_ms", 0).asInt64();
        msg.deleted_at_ms = txt_obj.get("deleted_at_ms", 0).asInt64();
        msg.created_at = txt_obj.get("created_at", 0).asInt64();
        if (isMember(txt_obj, "forward_meta"))
        {
            msg.forward_meta_json = JsonToCompactStringLocal(txt_obj["forward_meta"]);
        }
        if (msg.created_at <= 0)
        {
            msg.created_at = NowMs();
        }
        if (msg.msg_id.empty() || msg.content.empty())
        {
            rtdto.error = ErrorCodes::Error_Json;
            return result();
        }
        if (first_msg_id.empty())
        {
            first_msg_id = msg.msg_id;
        }
        pending_messages.push_back(msg);

        append(normalized,
               ChatHistoryOutput::ToJsonValue(ChatHistoryOutput::ChatPrivateOfflinePushMessageFromInfo(msg)));
    }

    const auto accept_ts = NowMs();
    rtdto.client_msg_id = first_msg_id;
    rtdto.accept_node = memochat::chatruntime::SelfServerName();
    rtdto.accept_ts = static_cast<int64_t>(accept_ts);
    rtdto.status = kafka_primary ? "accepted" : "persisted";
    if (!kafka_primary)
    {
        rtdto.text_array = normalized;
    }

    auto event_payload = ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateSendEventDto{
        .fromuid = uid,
        .touid = touid,
        .trace_id = root.get("trace_id", "").asString(),
        .request_id = root.get("request_id", "").asString(),
        .span_id = root.get("span_id", "").asString(),
        .event_id = first_msg_id,
        .accept_node = memochat::chatruntime::SelfServerName(),
        .accept_ts = static_cast<int64_t>(accept_ts)});
    event_payload["text_array"] = normalized;

    if (kafka_primary || kafka_shadow)
    {
        std::string publish_error;
        if (!_event_publisher ||
            !_event_publisher->PublishEvent(memochat::chatruntime::TopicPrivate(), event_payload, &publish_error))
        {
            if (kafka_primary)
            {
                rtdto.error = ErrorCodes::RPCFailed;
                rtdto.status = "failed";
                return result();
            }
            memolog::LogWarn("chat.private.shadow_publish_failed",
                             "private shadow publish failed",
                             {{"error", publish_error}, {"client_msg_id", first_msg_id}});
        }
    }

    if (kafka_primary)
    {
        return result();
    }

    for (const auto& msg : pending_messages)
    {
        if (!_message_repository || !_message_repository->SavePrivateMessage(msg))
        {
            rtdto.error = ErrorCodes::RPCFailed;
            rtdto.status = "failed";
            return result();
        }
    }

    const memochat::json::JsonValue rtvalue = ChatMessageCommand::ToJsonValue(rtdto);
    const bool notify_delivered =
        _delivery_gateway &&
        _delivery_gateway->TryPushPayload({touid}, ID_NOTIFY_TEXT_CHAT_MSG_REQ, rtvalue, uid, false);
    const auto route_fields =
        std::map<std::string, std::string>{{"from_uid", std::to_string(uid)},
                                           {"to_uid", std::to_string(touid)},
                                           {"msg_id", first_msg_id},
                                           {"notify_delivered", notify_delivered ? "true" : "false"}};
    if (notify_delivered)
    {
        memolog::LogInfo("chat.private.route", "private message notify dispatched", route_fields);
    }
    else
    {
        memolog::LogWarn("chat.private.route", "private message notify not delivered", route_fields);
    }
    return result();
}

void PrivateMessageService::HandleTextChatMessage(const std::shared_ptr<CSession>& session,
                                                  short msg_id,
                                                  const std::string& msg_data)
{
    SendMessageCommandResultLocal(session, TextChatMessage(BuildMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult PrivateMessageService::ForwardPrivateMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatMessageCommand::ChatPrivateForwardRequestFromJsonValue(root);
    const int from_uid = command.from_uid;
    const int peer_uid = command.peer_uid;
    const std::string& source_msg_id = command.source_msg_id;
    std::string client_msg_id = command.client_msg_id;

    memochat::json::JsonValue rtvalue =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateForwardResultDto{
            .error = ErrorCodes::Success,
            .fromuid = from_uid,
            .peer_uid = peer_uid,
            .touid = peer_uid,
            .client_msg_id = client_msg_id});

    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_FORWARD_PRIVATE_MSG_RSP,
                                    rtvalue
                                        .and_then(
                                            [](auto&& v)
                                            {
                                                return glz::write_json(v);
                                            })
                                        .value_or("{}")};
    };

    if (from_uid <= 0 || peer_uid <= 0 || source_msg_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_message_repository || !_message_repository->IsPrivateFriend(from_uid, peer_uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::shared_ptr<PrivateMessageInfo> source_msg;
    if (!_message_repository->FindPrivateMessageByClientId(source_msg_id, source_msg))
    {
        rtvalue["error"] = ErrorCodes::GroupNotFound;
        return result();
    }

    const int conv_uid_min = std::min(from_uid, peer_uid);
    const int conv_uid_max = std::max(from_uid, peer_uid);
    if (source_msg->conv_uid_min != conv_uid_min || source_msg->conv_uid_max != conv_uid_max)
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const int64_t now_ms = NowMs();
    if (client_msg_id.empty())
    {
        client_msg_id = std::to_string(from_uid) + "_" + std::to_string(now_ms);
        rtvalue["client_msg_id"] = client_msg_id;
    }

    PrivateMessageInfo info;
    info.msg_id = client_msg_id;
    info.conv_uid_min = conv_uid_min;
    info.conv_uid_max = conv_uid_max;
    info.from_uid = from_uid;
    info.to_uid = peer_uid;
    info.content = source_msg->content;
    info.reply_to_server_msg_id = source_msg->reply_to_server_msg_id;
    info.created_at = now_ms;

    memochat::json::JsonValue forward_meta;
    forward_meta["forwarded_from_msgid"] = source_msg_id;
    forward_meta["source_from_uid"] = source_msg->from_uid;
    forward_meta["source_conv_uid_min"] = source_msg->conv_uid_min;
    forward_meta["source_conv_uid_max"] = source_msg->conv_uid_max;
    forward_meta["source_created_at"] = static_cast<int64_t>(source_msg->created_at);
    if (!source_msg->forward_meta_json.empty())
    {
        memochat::json::JsonReader prev_forward_reader;
        memochat::json::JsonValue prev_forward_meta;
        if (prev_forward_reader.parse(source_msg->forward_meta_json, prev_forward_meta))
        {
            forward_meta["prev_forward_meta"] = prev_forward_meta;
        }
    }
    info.forward_meta_json = forward_meta
                                 .and_then(
                                     [](auto&& v)
                                     {
                                         return glz::write_json(v);
                                     })
                                 .value_or("{}");

    if (!_message_repository->SavePrivateMessage(info))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }

    const memochat::json::JsonValue msg_obj =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateForwardMessageDto{
            .msgid = info.msg_id,
            .content = info.content,
            .created_at = now_ms,
            .reply_to_server_msg_id = info.reply_to_server_msg_id,
            .forward_meta = forward_meta});
    rtvalue = ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateForwardResultDto{
        .error = ErrorCodes::Success,
        .fromuid = from_uid,
        .peer_uid = peer_uid,
        .touid = peer_uid,
        .client_msg_id = client_msg_id,
        .created_at = now_ms,
        .msg = msg_obj});

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(from_uid);
    text_msg_req.set_touid(peer_uid);
    auto* text_msg = text_msg_req.add_textmsgs();
    text_msg->set_msgid(info.msg_id);
    text_msg->set_msgcontent(info.content);

    const auto route = ResolveOnlineRoute(peer_uid, _session_registry, _online_route_store);
    if (route.kind == OnlineRouteKind::Offline || route.kind == OnlineRouteKind::Stale)
    {
        LogPrivateRouteLocal("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, "skipped", false);
        return result();
    }
    if (route.kind == OnlineRouteKind::Local && route.session)
    {
        route.session->Send(rtvalue
                                .and_then(
                                    [](auto&& v)
                                    {
                                        return glz::write_json(v);
                                    })
                                .value_or("{}"),
                            ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        LogPrivateRouteLocal("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, "n/a", true);
        return result();
    }

    const auto notify_rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(route.redis_server, text_msg_req, rtvalue);
    if (notify_rsp.error() != ErrorCodes::Success)
    {
        if (notify_rsp.error() == ErrorCodes::TargetOffline)
        {
            if (_online_route_store)
            {
                _online_route_store->ClearTrackedOnlineRoute(peer_uid, route.redis_server);
            }
        }
        LogPrivateRouteLocal("chat.private.forward.route",
                             from_uid,
                             peer_uid,
                             info.msg_id,
                             route,
                             std::to_string(notify_rsp.error()),
                             false);
        return result();
    }
    LogPrivateRouteLocal("chat.private.forward.route",
                         from_uid,
                         peer_uid,
                         info.msg_id,
                         route,
                         std::to_string(notify_rsp.error()),
                         true);
    return result();
}

void PrivateMessageService::HandleForwardPrivateMessage(const std::shared_ptr<CSession>& session,
                                                        short msg_id,
                                                        const std::string& msg_data)
{
    SendMessageCommandResultLocal(session,
                                  ForwardPrivateMessage(BuildMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult PrivateMessageService::PrivateReadAck(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int peer_uid = root.get("peer_uid", 0).asInt();
    int64_t read_ts = root.get("read_ts", 0).asInt64();
    const auto result = []()
    {
        return MessageCommandResult{0, "{}"};
    };
    if (uid <= 0 || peer_uid <= 0)
    {
        return result();
    }
    if (!_message_repository || !_message_repository->IsPrivateFriend(uid, peer_uid))
    {
        return result();
    }
    if (read_ts <= 0)
    {
        read_ts = NowMs();
    }
    _message_repository->UpsertPrivateReadState(uid, peer_uid, read_ts);

    const auto notify = ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateReadAckEventDto{
        .error = ErrorCodes::Success,
        .event = "private_read_ack",
        .fromuid = uid,
        .peer_uid = peer_uid,
        .read_ts = read_ts});
    _delivery_gateway->PushPayload({peer_uid}, ID_NOTIFY_PRIVATE_READ_ACK_REQ, notify);
    return result();
}

void PrivateMessageService::HandlePrivateReadAck(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendMessageCommandResultLocal(session, PrivateReadAck(BuildMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult PrivateMessageService::EditPrivateMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatMessageCommand::ChatPrivateEditRequestFromJsonValue(root);
    const int uid = command.uid;
    const int peer_uid = command.peer_uid;
    const std::string& target_msg_id = command.msgid;
    const std::string& content = command.content;
    const int64_t now_ms = NowMs();

    memochat::json::JsonValue rtvalue =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateMessageChangedResultDto{
            .error = ErrorCodes::Success,
            .fromuid = uid,
            .peer_uid = peer_uid,
            .msgid = target_msg_id,
            .content = content,
            .changed_at_ms = now_ms});
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_EDIT_PRIVATE_MSG_RSP,
                                    rtvalue
                                        .and_then(
                                            [](auto&& v)
                                            {
                                                return glz::write_json(v);
                                            })
                                        .value_or("{}")};
    };

    if (uid <= 0 || peer_uid <= 0 || target_msg_id.empty() || content.empty() || content.size() > 4096)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_message_repository || !_message_repository->IsPrivateFriend(uid, peer_uid))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return result();
    }
    if (!_message_repository->UpdatePrivateMessageContent(uid, peer_uid, target_msg_id, content, now_ms))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const memochat::json::JsonValue notify =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateMessageChangedEventDto{
            .error = ErrorCodes::Success,
            .event = "private_msg_edited",
            .fromuid = uid,
            .peer_uid = peer_uid,
            .msgid = target_msg_id,
            .content = content,
            .changed_at_ms = now_ms});
    _delivery_gateway->PushPayload({uid, peer_uid}, ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, notify);
    return result();
}

void PrivateMessageService::HandleEditPrivateMessage(const std::shared_ptr<CSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    SendMessageCommandResultLocal(session,
                                  EditPrivateMessage(BuildMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult PrivateMessageService::RevokePrivateMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatMessageCommand::ChatPrivateRevokeRequestFromJsonValue(root);
    const int uid = command.uid;
    const int peer_uid = command.peer_uid;
    const std::string& target_msg_id = command.msgid;
    const int64_t now_ms = NowMs();

    memochat::json::JsonValue rtvalue =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateMessageChangedResultDto{
            .error = ErrorCodes::Success,
            .fromuid = uid,
            .peer_uid = peer_uid,
            .msgid = target_msg_id,
            .content = "[消息已撤回]",
            .changed_at_ms = now_ms,
            .deleted = true});
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_REVOKE_PRIVATE_MSG_RSP,
                                    rtvalue
                                        .and_then(
                                            [](auto&& v)
                                            {
                                                return glz::write_json(v);
                                            })
                                        .value_or("{}")};
    };

    if (uid <= 0 || peer_uid <= 0 || target_msg_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_message_repository || !_message_repository->IsPrivateFriend(uid, peer_uid))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return result();
    }
    if (!_message_repository->RevokePrivateMessage(uid, peer_uid, target_msg_id, now_ms))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const memochat::json::JsonValue notify =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatPrivateMessageChangedEventDto{
            .error = ErrorCodes::Success,
            .event = "private_msg_revoked",
            .fromuid = uid,
            .peer_uid = peer_uid,
            .msgid = target_msg_id,
            .content = "[消息已撤回]",
            .changed_at_ms = now_ms,
            .deleted = true});
    _delivery_gateway->PushPayload({uid, peer_uid}, ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, notify);
    return result();
}

void PrivateMessageService::HandleRevokePrivateMessage(const std::shared_ptr<CSession>& session,
                                                       short msg_id,
                                                       const std::string& msg_data)
{
    SendMessageCommandResultLocal(session,
                                  RevokePrivateMessage(BuildMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult PrivateMessageService::PrivateHistory(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatHistoryCommand::ChatPrivateHistoryRequestFromJsonValue(root);
    const int uid = command.uid;
    const int peer_uid = command.peer_uid;
    const int64_t before_ts = command.before_ts;
    const std::string& before_msg_id = command.before_msg_id;
    const int limit = command.limit;

    memochat::json::JsonValue rtvalue = ChatHistoryCommand::ToJsonValue(ChatHistoryCommand::ChatPrivateHistoryResponseDto{
        .error = ErrorCodes::Success,
        .peer_uid = peer_uid,
        .has_more = false});
    rtvalue["messages"] = memochat::json::arrayValue;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_PRIVATE_HISTORY_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || peer_uid <= 0 || limit <= 0)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }

    std::vector<std::shared_ptr<PrivateMessageInfo>> messages;
    bool has_more = false;
    if (!_message_repository ||
        !_message_repository->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }
    rtvalue["has_more"] = has_more;
    int64_t max_peer_read_ts = 0;
    for (const auto& one : messages)
    {
        if (!one)
        {
            continue;
        }
        const memochat::json::JsonValue item =
            ChatHistoryOutput::ToJsonValue(ChatHistoryOutput::ChatPrivateHistoryMessageFromInfo(*one));
        append(rtvalue["messages"], item);
        if (one->from_uid == peer_uid && one->created_at > max_peer_read_ts)
        {
            max_peer_read_ts = one->created_at;
        }
    }
    if (max_peer_read_ts > 0)
    {
        _message_repository->UpsertPrivateReadState(uid, peer_uid, max_peer_read_ts);
    }
    return result();
}

void PrivateMessageService::HandlePrivateHistory(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendMessageCommandResultLocal(session, PrivateHistory(BuildMessageCommandRequestLocal(session, msg_id, msg_data)));
}
