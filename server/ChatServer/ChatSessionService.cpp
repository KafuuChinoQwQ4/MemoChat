#include "ChatSessionService.h"

#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "CServer.h"
#include "CSession.h"
#include "ChatUserSupport.h"
#include "LogicSystem.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "auth/ChatLoginTicket.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <chrono>
#include <json/json.h>

namespace {
int64_t NowMsLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string GetChatAuthSecretLocal() {
    static const std::string secret = []() {
        auto& cfg = ConfigMgr::Inst();
        auto value = cfg.GetValue("ChatAuth", "HmacSecret");
        if (value.empty()) {
            value = "memochat-dev-chat-secret";
        }
        return value;
    }();
    return secret;
}

void RepairOnlineRouteStateLocal(int uid, const std::shared_ptr<CSession>& session, const std::string& server_name) {
    if (uid <= 0 || !session || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
    RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
    RedisMgr::GetInstance()->SAdd(std::string(SERVER_ONLINE_USERS_PREFIX) + server_name, uid_str);
}
}

ChatSessionService::ChatSessionService(LogicSystem& logic)
    : _logic(logic) {
}

void ChatSessionService::HandleLogin(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data)
{
    const auto login_start_ms = NowMsLocal();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();
    const int protocol_version = root.get("protocol_version", 1).asInt();
    const auto verify_start_ms = NowMsLocal();
    auto trace_id = root.get("trace_id", "").asString();
    if (trace_id.empty()) {
        trace_id = memolog::TraceContext::NewId();
    }
    memolog::TraceContext::SetTraceId(trace_id);
    memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
    memolog::TraceContext::SetUid(std::to_string(uid));
    memolog::TraceContext::SetSessionId(session->GetSessionId());
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    memolog::LogInfo("chat.login.received", "chat login request received",
        {{"uid", std::to_string(uid)}, {"session_id", session->GetSessionId()}, {"tcp_msg_id", std::to_string(msg_id)}});

    Json::Value rtvalue;
    Defer defer([&rtvalue, session]() {
        session->Send(rtvalue.toStyledString(), MSG_CHAT_LOGIN_RSP);
    });
    rtvalue["trace_id"] = trace_id;
    rtvalue["protocol_version"] = 2;

    memochat::auth::ChatLoginTicketClaims ticket_claims;
    const auto self_server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
    std::string uid_str = std::to_string(uid);
    std::string relation_bootstrap_mode = "inline";
    if (protocol_version >= 3) {
        relation_bootstrap_mode = "explicit_pull";
        rtvalue["protocol_version"] = 3;
        std::string ticket_error;
        const auto login_ticket = root.get("login_ticket", "").asString();
        if (!memochat::auth::DecodeAndVerifyTicket(login_ticket, GetChatAuthSecretLocal(), ticket_claims, &ticket_error)) {
            rtvalue["error"] = ErrorCodes::ChatTicketInvalid;
            memolog::LogWarn("chat.login.failed", "chat ticket invalid",
                {{"error_code", std::to_string(ErrorCodes::ChatTicketInvalid)}, {"detail", ticket_error}});
            return;
        }
        if (ticket_claims.expire_at_ms > 0 && ticket_claims.expire_at_ms < NowMsLocal()) {
            rtvalue["error"] = ErrorCodes::ChatTicketExpired;
            memolog::LogWarn("chat.login.failed", "chat ticket expired",
                {{"error_code", std::to_string(ErrorCodes::ChatTicketExpired)}});
            return;
        }
        if (!ticket_claims.target_server.empty() && ticket_claims.target_server != self_server_name) {
            rtvalue["error"] = ErrorCodes::ChatServerMismatch;
            memolog::LogWarn("chat.login.failed", "chat ticket target server mismatch",
                {{"error_code", std::to_string(ErrorCodes::ChatServerMismatch)},
                 {"ticket_target_server", ticket_claims.target_server},
                 {"self_server", self_server_name}});
            return;
        }
        uid = ticket_claims.uid;
        uid_str = std::to_string(uid);
        memolog::TraceContext::SetUid(uid_str);
    } else if (protocol_version != 2) {
        rtvalue["error"] = ErrorCodes::ProtocolVersionMismatch;
        return;
    }

    if (protocol_version < 3) {
        std::string token_key = USERTOKENPREFIX + uid_str;
        std::string token_value;
        if (!RedisMgr::GetInstance()->Get(token_key, token_value)) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            memolog::LogWarn("chat.login.failed", "uid invalid", {{"error_code", std::to_string(ErrorCodes::UidInvalid)}});
            return;
        }
        if (token_value != token) {
            rtvalue["error"] = ErrorCodes::TokenInvalid;
            memolog::LogWarn("chat.login.failed", "token invalid", {{"error_code", std::to_string(ErrorCodes::TokenInvalid)}});
            return;
        }
    }

    rtvalue["error"] = ErrorCodes::Success;
    auto user_info = std::make_shared<UserInfo>();
    if (protocol_version >= 3) {
        user_info->uid = ticket_claims.uid;
        user_info->user_id = ticket_claims.user_id;
        user_info->name = ticket_claims.name;
        user_info->nick = ticket_claims.nick;
        user_info->icon = ticket_claims.icon;
        user_info->desc = ticket_claims.desc;
        user_info->email = ticket_claims.email;
        user_info->sex = ticket_claims.sex;
    } else {
        std::string base_key = USER_BASE_INFO + uid_str;
        if (!chatusersupport::GetBaseInfo(base_key, uid, user_info)) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            memolog::LogWarn("chat.login.failed", "user base info not found", {{"error_code", std::to_string(ErrorCodes::UidInvalid)}});
            return;
        }
    }
    rtvalue["uid"] = uid;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
    rtvalue["user_id"] = user_info->user_id;

    if (protocol_version < 3) {
        _logic.AppendRelationBootstrapJson(uid, rtvalue);
    }

    const auto ticket_verify_ms = NowMsLocal() - verify_start_ms;
    const auto attach_start_ms = NowMsLocal();
    const auto server_name = self_server_name;
    {
        auto lock_key = LOCK_PREFIX + uid_str;
        auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
        Defer defer2([identifier, lock_key]() {
            RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
        });

        std::string uid_ip_value;
        bool b_ip = RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, uid_ip_value);
        if (b_ip) {
            auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];
            if (uid_ip_value == self_name) {
                auto old_session = UserMgr::GetInstance()->GetSession(uid);
                if (old_session) {
                    old_session->NotifyOffline(uid);
                    _logic._p_server->ClearSession(old_session->GetSessionId());
                }
            } else {
                KickUserReq kick_req;
                kick_req.set_uid(uid);
                ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
            }
        }

        session->SetUserId(uid);
        RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
        UserMgr::GetInstance()->SetUserSession(uid, session);
        RepairOnlineRouteStateLocal(uid, session, server_name);

        // 验证 Redis 状态是否正确设置
        std::string verify_server;
        RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, verify_server);
        memolog::LogInfo("chat.login.redis_verify", "verified redis online status",
            {{"uid", uid_str}, {"expected_server", server_name}, {"actual_server", verify_server}});
    }

    const auto session_attach_ms = NowMsLocal() - attach_start_ms;
    memolog::LogInfo("chat.login.succeeded", "chat login success",
        {{"uid", std::to_string(uid)}, {"session_id", session->GetSessionId()},
         {"login_protocol_version", std::to_string(protocol_version >= 3 ? 3 : 2)},
         {"ticket_verify_ms", std::to_string(ticket_verify_ms)}, {"session_attach_ms", std::to_string(session_attach_ms)},
         {"relation_bootstrap_ms", "0"}, {"relation_bootstrap_mode", relation_bootstrap_mode},
         {"chat_login_total_ms", std::to_string(NowMsLocal() - login_start_ms)}});
    memolog::LogInfo("login.stage.summary", "chat login stage summary",
        {{"uid", std::to_string(uid)}, {"login_protocol_version", std::to_string(protocol_version >= 3 ? 3 : 2)},
         {"ticket_verify_ms", std::to_string(ticket_verify_ms)}, {"session_attach_ms", std::to_string(session_attach_ms)},
         {"relation_bootstrap_ms", "0"}, {"relation_bootstrap_mode", relation_bootstrap_mode},
         {"chat_login_total_ms", std::to_string(NowMsLocal() - login_start_ms)}});
}

void ChatSessionService::HandleRelationBootstrap(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data)
{
    const auto bootstrap_start_ms = NowMsLocal();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto trace_id = root.get("trace_id", "").asString();
    if (trace_id.empty()) {
        trace_id = memolog::TraceContext::NewId();
    }
    memolog::TraceContext::SetTraceId(trace_id);
    memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
    if (session) {
        memolog::TraceContext::SetSessionId(session->GetSessionId());
    }
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });

    Json::Value rtvalue;
    rtvalue["trace_id"] = trace_id;
    rtvalue["protocol_version"] = root.get("protocol_version", 3).asInt();
    const int uid = session ? session->GetUserId() : 0;
    if (uid <= 0) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        if (session) {
            session->Send(rtvalue.toStyledString(), ID_GET_RELATION_BOOTSTRAP_RSP);
        }
        return;
    }

    memolog::TraceContext::SetUid(std::to_string(uid));
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    _logic.AppendRelationBootstrapJson(uid, rtvalue);
    if (session) {
        session->Send(rtvalue.toStyledString(), ID_GET_RELATION_BOOTSTRAP_RSP);
    }
    memolog::LogInfo("chat.relation_bootstrap.succeeded", "relation bootstrap fetched",
        {{"uid", std::to_string(uid)}, {"tcp_msg_id", std::to_string(msg_id)},
         {"relation_bootstrap_ms", std::to_string(NowMsLocal() - bootstrap_start_ms)},
         {"relation_bootstrap_mode", "explicit_pull"}});
}

void ChatSessionService::HandleHeartbeat(const std::shared_ptr<CSession>& session, short, const std::string&)
{
    // 更新会话的心跳时间戳
    session->UpdateHeartbeat();

    // 获取当前登录的用户 ID
    const auto uid = session->GetUserId();
    if (uid > 0) {
        const auto uid_str = std::to_string(uid);
        const auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];

        // 检查 UserMgr 中是否已经有正确的会话映射
        auto existing_session = UserMgr::GetInstance()->GetSession(uid);
        if (!existing_session || existing_session.get() != session.get()) {
            // 修复 Redis 在线状态，确保跨服务器查询时能找到用户
            RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, self_name);
            RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
            RedisMgr::GetInstance()->SAdd(std::string(SERVER_ONLINE_USERS_PREFIX) + self_name, uid_str);

            // 修复本地 UserMgr 会话映射
            UserMgr::GetInstance()->SetUserSession(uid, session);

            memolog::LogInfo("chat.heartbeat.session_repair", "repaired session mapping",
                {{"uid", uid_str}, {"session_id", session->GetSessionId()}});
        } else {
            // 仅刷新 Redis 在线状态
            RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, self_name);
            RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
            RedisMgr::GetInstance()->SAdd(std::string(SERVER_ONLINE_USERS_PREFIX) + self_name, uid_str);
        }
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}
