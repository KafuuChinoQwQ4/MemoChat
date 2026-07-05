#include "ChatSessionService.hpp"

#include "ChatGrpcClient.hpp"
#include "ChatHistoryOutputDtos.hpp"
#include "CServer.hpp"
#include "IChatSession.hpp"
#include "LogicSystem.hpp"
#include "SessionSendSupport.hpp"
#include "auth/ChatLoginTicket.hpp"
#include "json/GlazeCompat.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <thread>

namespace
{
int64_t NowMsLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

// Compact JSON for wire (Qt QJsonDocument is strict; avoid pretty-print / stray whitespace issues).
std::string JsonValueToWireString(const memochat::json::JsonValue& v)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, v);
}

int LoginTicketJtiTtlSec(const memochat::auth::ChatLoginTicketClaims& claims)
{
    if (claims.jti.empty())
    {
        return 0;
    }
    const int64_t remaining_ms = claims.expire_at_ms - NowMsLocal();
    if (remaining_ms <= 0)
    {
        return 0;
    }
    return static_cast<int>(std::clamp<int64_t>((remaining_ms + 999) / 1000, 1, 300));
}
} // namespace

ChatSessionService::ChatSessionService(LogicSystem& logic,
                                       ISessionRegistry* session_registry,
                                       IOnlineRouteStore* online_route_store,
                                       IRelationQueryService* relation_query_service,
                                       IRelationRepository* relation_repository,
                                       IChatSessionConfig* session_config,
                                       IChatSessionRepository* session_repository)
    : _logic(logic)
    , _session_registry(session_registry)
    , _online_route_store(online_route_store)
    , _relation_query_service(relation_query_service)
    , _relation_repository(relation_repository)
    , _session_config(session_config)
    , _session_repository(session_repository)
{
}

void ChatSessionService::HandleLogin(const std::shared_ptr<IChatSession>& session,
                                     short msg_id,
                                     const std::string& msg_data)
{
    const auto login_start_ms = NowMsLocal();
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int protocol_version = memochat::json::glaze_safe_get<int>(root, "protocol_version", 0);
    auto uid = 0;
    const auto verify_start_ms = NowMsLocal();
    auto trace_id = root.get("trace_id", "").asString();
    if (trace_id.empty())
    {
        trace_id = memolog::TraceContext::NewId();
    }
    memolog::TraceContext::SetTraceId(trace_id);
    memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
    memolog::TraceContext::SetUid(std::to_string(uid));
    memolog::TraceContext::SetSessionId(session->sessionId());
    Defer clear_trace(
        []()
        {
            memolog::TraceContext::Clear();
        });
    const bool verbose_login_logs =
        !_session_config || _session_config->FeatureFlagDefaultTrue("chat_login_verbose_logs");
    if (verbose_login_logs)
    {
        memolog::LogInfo("chat.login.received",
                         "chat login request received",
                         {{"uid", std::to_string(uid)},
                          {"session_id", session->sessionId()},
                          {"tcp_msg_id", std::to_string(msg_id)}});
    }

    memochat::json::JsonValue rtvalue;
    Defer defer(
        [&rtvalue, session]()
        {
            session->send(JsonValueToWireString(rtvalue), MSG_CHAT_LOGIN_RSP);
        });
    rtvalue["trace_id"] = trace_id;
    rtvalue["protocol_version"] = 3;

    memochat::auth::ChatLoginTicketClaims ticket_claims;
    const auto self_server_name = _online_route_store
                                      ? _online_route_store->SelfServerName()
                                      : (_session_config ? _session_config->SelfServerName() : std::string());
    const std::string relation_bootstrap_mode = "explicit_pull";
    if (protocol_version != 3)
    {
        rtvalue["error"] = ErrorCodes::ProtocolVersionMismatch;
        return;
    }

    std::string ticket_error;
    const auto login_ticket = root.get("login_ticket", "").asString();
    if (!memochat::auth::DecodeAndVerifyTicket(login_ticket,
                                               _session_config ? _session_config->ChatAuthSecret()
                                                               : std::string("memochat-dev-chat-secret"),
                                               ticket_claims,
                                               &ticket_error))
    {
        rtvalue["error"] = ErrorCodes::ChatTicketInvalid;
        memolog::LogWarn("chat.login.failed",
                         "chat ticket invalid",
                         {{"error_code", std::to_string(ErrorCodes::ChatTicketInvalid)}, {"detail", ticket_error}});
        return;
    }
    if (ticket_claims.protocol_version != 3)
    {
        rtvalue["error"] = ErrorCodes::ProtocolVersionMismatch;
        memolog::LogWarn("chat.login.failed",
                         "chat ticket protocol version mismatch",
                         {{"error_code", std::to_string(ErrorCodes::ProtocolVersionMismatch)},
                          {"ticket_protocol_version", std::to_string(ticket_claims.protocol_version)}});
        return;
    }
    if (ticket_claims.expire_at_ms > 0 && ticket_claims.expire_at_ms < NowMsLocal())
    {
        rtvalue["error"] = ErrorCodes::ChatTicketExpired;
        memolog::LogWarn("chat.login.failed",
                         "chat ticket expired",
                         {{"error_code", std::to_string(ErrorCodes::ChatTicketExpired)}});
        return;
    }
    if (!ticket_claims.target_server.empty() && ticket_claims.target_server != self_server_name)
    {
        rtvalue["error"] = ErrorCodes::ChatServerMismatch;
        memolog::LogWarn("chat.login.failed",
                         "chat ticket target server mismatch",
                         {{"error_code", std::to_string(ErrorCodes::ChatServerMismatch)},
                          {"ticket_target_server", ticket_claims.target_server},
                          {"self_server", self_server_name}});
        return;
    }
    const int ticket_jti_ttl_sec = LoginTicketJtiTtlSec(ticket_claims);
    if (!_session_repository ||
        !_session_repository->ConsumeLoginTicketJti(ticket_claims.jti, ticket_claims.uid, ticket_jti_ttl_sec))
    {
        rtvalue["error"] = ErrorCodes::ChatTicketInvalid;
        memolog::LogWarn("chat.login.failed",
                         "chat ticket replayed or missing jti",
                         {{"error_code", std::to_string(ErrorCodes::ChatTicketInvalid)}});
        return;
    }
    uid = ticket_claims.uid;
    memolog::TraceContext::SetUid(std::to_string(uid));

    rtvalue["error"] = ErrorCodes::Success;
    auto user_info = std::make_shared<UserInfo>();
    user_info->uid = ticket_claims.uid;
    user_info->user_id = ticket_claims.user_id;
    user_info->name = ticket_claims.name;
    user_info->nick = ticket_claims.nick;
    user_info->icon = ticket_claims.icon;
    user_info->desc = ticket_claims.desc;
    user_info->email = ticket_claims.email;
    user_info->sex = ticket_claims.sex;
    rtvalue["uid"] = uid;
    // Do not echo password hash over chat transport (security + avoids rare JSON/Qt parse issues).
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
    rtvalue["user_id"] = user_info->user_id;

    const auto ticket_verify_ms = NowMsLocal() - verify_start_ms;
    const auto attach_start_ms = NowMsLocal();
    const auto server_name = self_server_name;
    auto attach_session = [&]()
    {
        session->setUserId(uid);
        if (_session_registry)
        {
            _session_registry->BindSession(uid, session);
        }
        if (_online_route_store)
        {
            _online_route_store->RepairOnlineRoute(uid, session);
        }
        session->markOnlineRouteRefreshed(std::time(nullptr));
    };
    {
        if (!_session_config || _session_config->FeatureFlagDefaultTrue("chat_login_duplicate_session_check"))
        {
            auto identifier = _session_repository ? _session_repository->AcquireDuplicateLoginLock(uid) : std::string();
            Defer defer2(
                [this, uid, identifier]()
                {
                    if (_session_repository)
                    {
                        _session_repository->ReleaseDuplicateLoginLock(uid, identifier);
                    }
                });

            const auto uid_ip_value = _online_route_store ? _online_route_store->FindUserServer(uid) : std::string();
            if (!uid_ip_value.empty())
            {
                const auto self_name = _online_route_store
                                           ? _online_route_store->SelfServerName()
                                           : (_session_config ? _session_config->SelfServerName() : std::string());
                if (uid_ip_value == self_name)
                {
                    auto old_session = _session_registry ? _session_registry->FindSession(uid) : nullptr;
                    if (old_session)
                    {
                        SendChatSessionOfflineNotification(old_session, uid);
                        _logic._p_server->ClearSession(old_session->sessionId());
                    }
                }
                else
                {
                    KickUserReq kick_req;
                    kick_req.set_uid(uid);
                    ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
                }
            }
            attach_session();
        }
        else
        {
            attach_session();
        }
    }

    const auto session_attach_ms = NowMsLocal() - attach_start_ms;
    if (verbose_login_logs)
    {
        memolog::LogInfo("chat.login.succeeded",
                         "chat login success",
                         {{"uid", std::to_string(uid)},
                          {"session_id", session->sessionId()},
                          {"login_protocol_version", "3"},
                          {"ticket_verify_ms", std::to_string(ticket_verify_ms)},
                          {"session_attach_ms", std::to_string(session_attach_ms)},
                          {"relation_bootstrap_ms", "0"},
                          {"relation_bootstrap_mode", relation_bootstrap_mode},
                          {"chat_login_total_ms", std::to_string(NowMsLocal() - login_start_ms)}});
        memolog::LogInfo("login.stage.summary",
                         "chat login stage summary",
                         {{"uid", std::to_string(uid)},
                          {"login_protocol_version", "3"},
                          {"ticket_verify_ms", std::to_string(ticket_verify_ms)},
                          {"session_attach_ms", std::to_string(session_attach_ms)},
                          {"relation_bootstrap_ms", "0"},
                          {"relation_bootstrap_mode", relation_bootstrap_mode},
                          {"chat_login_total_ms", std::to_string(NowMsLocal() - login_start_ms)}});
    }

    if (!_session_config || _session_config->FeatureFlagDefaultTrue("chat_login_offline_push"))
    {
        std::thread(
            [this, session, uid]()
            {
                const int delay_ms = _session_config ? _session_config->LoginOfflinePushDelayMs() : 0;
                if (delay_ms > 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                }
                PushOfflineMessages(session, uid);
            })
            .detach();
    }
}

void ChatSessionService::HandleRelationBootstrap(const std::shared_ptr<IChatSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    const auto bootstrap_start_ms = NowMsLocal();
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    auto trace_id = root.get("trace_id", "").asString();
    if (trace_id.empty())
    {
        trace_id = memolog::TraceContext::NewId();
    }
    memolog::TraceContext::SetTraceId(trace_id);
    memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
    if (session)
    {
        memolog::TraceContext::SetSessionId(session->sessionId());
    }
    Defer clear_trace(
        []()
        {
            memolog::TraceContext::Clear();
        });

    memochat::json::JsonValue rtvalue;
    rtvalue["trace_id"] = trace_id;
    rtvalue["protocol_version"] = root.get("protocol_version", 3).asInt();
    const int uid = session ? session->userId() : 0;
    if (uid <= 0)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        if (session)
        {
            session->send(JsonValueToWireString(rtvalue), ID_GET_RELATION_BOOTSTRAP_RSP);
        }
        return;
    }

    memolog::TraceContext::SetUid(std::to_string(uid));
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    if (_relation_query_service)
    {
        _relation_query_service->AppendRelationBootstrapJson(uid, rtvalue);
    }
    if (session)
    {
        session->send(JsonValueToWireString(rtvalue), ID_GET_RELATION_BOOTSTRAP_RSP);
    }
    memolog::LogInfo("chat.relation_bootstrap.succeeded",
                     "relation bootstrap fetched",
                     {{"uid", std::to_string(uid)},
                      {"tcp_msg_id", std::to_string(msg_id)},
                      {"relation_bootstrap_ms", std::to_string(NowMsLocal() - bootstrap_start_ms)},
                      {"relation_bootstrap_mode", "explicit_pull"}});
}

void ChatSessionService::HandleHeartbeat(const std::shared_ptr<IChatSession>& session, short, const std::string&)
{
    if (!session)
    {
        return;
    }

    session->updateHeartbeat();

    const auto uid = session->userId();
    if (uid > 0)
    {
        const auto uid_str = std::to_string(uid);
        const auto now = std::time(nullptr);

        auto existing_session = _session_registry ? _session_registry->FindSession(uid) : nullptr;
        if (!existing_session || existing_session.get() != session.get())
        {
            if (_online_route_store)
            {
                _online_route_store->RepairOnlineRoute(uid, session);
            }
            session->markOnlineRouteRefreshed(now);
            if (_session_registry)
            {
                _session_registry->BindSession(uid, session);
            }
            memolog::LogInfo("chat.heartbeat.session_repair",
                             "repaired session mapping",
                             {{"uid", uid_str}, {"session_id", session->sessionId()}});
        }
        else if (session->tryMarkOnlineRouteRefreshDue(now,
                                                       _session_config ? _session_config->HeartbeatRouteRefreshSec()
                                                                       : 15))
        {
            if (_online_route_store)
            {
                _online_route_store->RepairOnlineRoute(uid, session);
            }
        }
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    session->send(JsonValueToWireString(rtvalue), ID_HEARTBEAT_RSP);
}

void ChatSessionService::PushOfflineMessages(const std::shared_ptr<IChatSession>& session, int uid)
{
    try
    {
        if (!session)
        {
            memolog::LogWarn("chat.login.offline_push_skipped",
                             "offline push skipped for null session",
                             {{"uid", std::to_string(uid)}});
            return;
        }

        constexpr int kOfflineBatchSize = 50;
        constexpr int kMaxOfflineBatches = 10;

        memolog::LogInfo("chat.login.offline_push_start",
                         "starting offline message push",
                         {{"uid", std::to_string(uid)}, {"session_id", session->sessionId()}});

        int pushed_total = 0;
        int64_t pagination_cursor_ts = 0;
        std::string pagination_cursor_msg_id;

        for (int batch = 0; batch < kMaxOfflineBatches; ++batch)
        {
            std::vector<std::shared_ptr<PrivateMessageInfo>> messages;
            if (!_session_repository ||
                !_session_repository->GetUndeliveredPrivateMessages(uid,
                                                                    pagination_cursor_ts,
                                                                    pagination_cursor_msg_id,
                                                                    kOfflineBatchSize,
                                                                    messages) ||
                messages.empty())
            {
                break;
            }

            // 按 from_uid 分组
            std::map<int, std::vector<std::shared_ptr<PrivateMessageInfo>>> msgs_by_sender;
            for (const auto& msg : messages)
            {
                if (msg && msg->from_uid != uid)
                {
                    msgs_by_sender[msg->from_uid].push_back(msg);
                    pagination_cursor_ts = msg->created_at;
                    pagination_cursor_msg_id = msg->msg_id;
                }
            }

            for (const auto& [sender_uid, sender_msgs] : msgs_by_sender)
            {
                std::string sender_public_user_id;
                if (_relation_repository)
                {
                    const auto sender_info = _relation_repository->GetUserByUid(sender_uid);
                    if (sender_info)
                    {
                        sender_public_user_id = sender_info->user_id;
                    }
                }
                memochat::json::JsonValue text_array(memochat::json::array_t{});
                for (const auto& msg : sender_msgs)
                {
                    msg->from_user_id = sender_public_user_id;
                    const memochat::json::JsonValue element = memochat::chat::history::output::ToJsonValue(
                        memochat::chat::history::output::ChatPrivateOfflinePushMessageFromInfo(*msg));
                    append(text_array, element);
                }

                const memochat::json::JsonValue notify = memochat::chat::history::output::ToJsonValue(
                    memochat::chat::history::output::ChatPrivateOfflinePushNotifyDto{.error = ErrorCodes::Success,
                                                                                     .fromuid = sender_uid,
                                                                                     .touid = uid,
                                                                                     .from_user_id =
                                                                                         sender_public_user_id,
                                                                                     .text_array = text_array});

                session->send(JsonValueToWireString(notify), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
                pushed_total += static_cast<int>(sender_msgs.size());

                memolog::LogInfo("chat.login.offline_push_batch",
                                 "pushed offline messages batch",
                                 {{"uid", std::to_string(uid)},
                                  {"from_uid", std::to_string(sender_uid)},
                                  {"count", std::to_string(sender_msgs.size())}});
            }

            if (static_cast<int>(messages.size()) < kOfflineBatchSize)
            {
                break;
            }
        }

        if (pushed_total <= 0)
        {
            memolog::LogInfo("chat.login.offline_push_skipped", "no unread messages", {{"uid", std::to_string(uid)}});
            return;
        }

        memolog::LogInfo("chat.login.offline_push_done",
                         "offline message push completed",
                         {{"uid", std::to_string(uid)},
                          {"session_id", session->sessionId()},
                          {"pushed_total", std::to_string(pushed_total)}});
    }
    catch (const std::exception& e)
    {
        memolog::LogError("chat.login.offline_push_failed",
                          "offline message push failed",
                          {{"uid", std::to_string(uid)}, {"error", e.what()}});
    }
    catch (...)
    {
        memolog::LogError("chat.login.offline_push_failed",
                          "offline message push failed with unknown exception",
                          {{"uid", std::to_string(uid)}});
    }
}
