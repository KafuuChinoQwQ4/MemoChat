#include "services/auth/AuthService.h"

#include "AuthCache.h"
#include "AuthLoginSupport.h"
#include "AuthVerifyClient.h"
#include "GateAsyncSideEffects.h"
#include "GateWorkerPool.h"
#include "auth/ChatLoginTicket.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include "services/account/AccountPersistence.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>

#include "json/GlazeCompat.h"

namespace memochat::gate::services::auth
{

namespace account = memochat::gate::services::account;
using memochat::gate::core::AuthCache;

namespace
{

void WriteJson(memochat::gate::routing::GateResponse& response, const memochat::json::JsonValue& root)
{
    response.status = 200;
    response.content_type = "application/json";
    response.body = root.toStyledString();
}

} // namespace

AuthService& AuthService::Instance()
{
    static AuthService instance;
    return instance;
}

bool AuthService::HandleGetVarifyCode(const memochat::gate::routing::GateRequest& request,
                                      memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        memolog::LogWarn("gate.get_varifycode.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    if (!isMember(src_root, "email"))
    {
        memolog::LogWarn("gate.get_varifycode.invalid_body", "email is missing");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = src_root["email"].asString();
    const auto result = memochat::gate::core::AuthVerifyClient::Instance().RequestVerifyCode(email);
    root["error"] = result.error;
    root["email"] = src_root["email"];
    memolog::LogInfo("gate.get_varifycode",
                     "verify code requested",
                     {{"route", "/get_varifycode"}, {"email", email}, {"error_code", std::to_string(result.error)}});
    WriteJson(response, root);
    return true;
}

bool AuthService::HandleResetPwd(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        memolog::LogWarn("gate.reset_pwd.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = src_root["email"].asString();
    const auto name = src_root["user"].asString();
    const auto pwd = src_root["passwd"].asString();

    std::string varify_code;
    if (!AuthCache::Instance().GetVerificationCode(src_root["email"].asString(), varify_code))
    {
        memolog::LogWarn("gate.reset_pwd.failed", "verify code expired", {{"email", email}});
        root["error"] = ErrorCodes::VarifyExpired;
        WriteJson(response, root);
        return true;
    }

    if (varify_code != src_root["varifycode"].asString())
    {
        memolog::LogWarn("gate.reset_pwd.failed", "verify code mismatch", {{"email", email}});
        root["error"] = ErrorCodes::VarifyCodeErr;
        WriteJson(response, root);
        return true;
    }

    auto& account_persistence = account::AccountPersistence::Instance();
    if (!account_persistence.EmailMatchesUser(name, email))
    {
        memolog::LogWarn("gate.reset_pwd.failed", "user email mismatch", {{"email", email}, {"name", name}});
        root["error"] = ErrorCodes::EmailNotMatch;
        WriteJson(response, root);
        return true;
    }

    if (!account_persistence.UpdatePassword(email, pwd))
    {
        memolog::LogWarn("gate.reset_pwd.failed", "password update failed", {{"email", email}});
        root["error"] = ErrorCodes::PasswdUpFailed;
        WriteJson(response, root);
        return true;
    }

    memolog::LogInfo("gate.reset_pwd", "password updated", {{"email", email}});
    gateauthsupport::InvalidateLoginCacheByEmail(email);
    GateAsyncSideEffects::Instance().PublishCacheInvalidate(email, name, "reset_pwd");
    root["error"] = 0;
    root["email"] = email;
    root["user"] = name;
    root["passwd"] = pwd;
    root["varifycode"] = src_root["varifycode"].asString();
    WriteJson(response, root);
    return true;
}

bool AuthService::HandleUserRegister(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        memolog::LogWarn("gate.user_register.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = src_root["email"].asString();
    const auto name = src_root["user"].asString();
    const auto pwd = src_root["passwd"].asString();
    const auto confirm = src_root["confirm"].asString();
    const auto icon = src_root["icon"].asString();

    if (pwd != confirm)
    {
        memolog::LogWarn("gate.user_register.failed", "password mismatch", {{"email", email}});
        root["error"] = ErrorCodes::PasswdErr;
        WriteJson(response, root);
        return true;
    }

    std::string varify_code;
    if (!AuthCache::Instance().GetVerificationCode(src_root["email"].asString(), varify_code))
    {
        memolog::LogWarn("gate.user_register.failed", "verify code expired", {{"email", email}});
        root["error"] = ErrorCodes::VarifyExpired;
        WriteJson(response, root);
        return true;
    }

    if (varify_code != src_root["varifycode"].asString())
    {
        memolog::LogWarn("gate.user_register.failed", "verify code mismatch", {{"email", email}});
        root["error"] = ErrorCodes::VarifyCodeErr;
        WriteJson(response, root);
        return true;
    }

    auto& account_persistence = account::AccountPersistence::Instance();
    const int uid = account_persistence.RegisterUser(name, email, pwd, icon);
    if (uid == 0 || uid == -1)
    {
        memolog::LogWarn("gate.user_register.failed", "user or email exists", {{"email", email}, {"name", name}});
        root["error"] = ErrorCodes::UserExist;
        WriteJson(response, root);
        return true;
    }

    root["error"] = 0;
    root["uid"] = uid;
    root["user_id"] = account_persistence.GetUserPublicId(uid);
    root["email"] = email;
    root["user"] = name;
    root["passwd"] = pwd;
    root["confirm"] = confirm;
    root["icon"] = icon;
    root["varifycode"] = src_root["varifycode"].asString();
    gateauthsupport::UserInfo cached_user;
    cached_user.uid = uid;
    cached_user.user_id = root["user_id"].asString();
    cached_user.name = name;
    cached_user.email = email;
    cached_user.pwd = pwd;
    cached_user.nick = name;
    cached_user.icon = icon;
    cached_user.desc = "";
    cached_user.sex = src_root.get("sex", 0).asInt();
    gateauthsupport::CacheLoginProfile(email, cached_user);
    GateAsyncSideEffects::Instance()
        .PublishUserProfileChanged(uid, root["user_id"].asString(), email, name, name, icon, cached_user.sex);
    memolog::LogInfo("gate.user_register", "user registered", {{"email", email}, {"uid", std::to_string(uid)}});
    WriteJson(response, root);
    return true;
}

bool AuthService::HandleUserLogin(const memochat::gate::routing::GateRequest& request,
                                  memochat::gate::routing::GateResponse& response)
{
    const auto login_start_ms = gateauthsupport::NowMs();
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    root["trace_id"] = request.trace_id;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        memolog::LogWarn("gate.user_login.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = src_root["email"].asString();
    const auto pwd = src_root["passwd"].asString();
    const auto client_ver = src_root.get("client_ver", "").asString();
    root["min_version"] = gateauthsupport::MinClientVersion();
    root["feature_group_chat"] = true;
    memolog::LogInfo("gate.user_login.version_check",
                     "version check",
                     {{"client_ver", client_ver}, {"min_ver", gateauthsupport::MinClientVersion()}});
    if (!gateauthsupport::IsClientVersionAllowed(client_ver, gateauthsupport::MinClientVersion()))
    {
        root["error"] = ErrorCodes::ClientVersionTooLow;
        memolog::LogWarn("gate.user_login.failed",
                         "client version too low",
                         {{"email", email}, {"error_code", std::to_string(ErrorCodes::ClientVersionTooLow)}});
        WriteJson(response, root);
        return true;
    }

    gateauthsupport::UserInfo userInfo;
    const auto mysql_start_ms = gateauthsupport::NowMs();
    gateauthsupport::UserInfo tempUser;
    bool login_cache_hit = gateauthsupport::TryLoadCachedLoginProfile(email, pwd, tempUser);
    if (login_cache_hit)
    {
        userInfo = tempUser;
        gateauthsupport::RefreshLoginProfileFromDb(email, userInfo);
    }
    bool pwd_valid = login_cache_hit;
    int64_t mysql_check_pwd_ms = 0;
    if (!pwd_valid)
    {
        account::AccountProfile dbUser;
        pwd_valid = account::AccountPersistence::Instance().CheckPassword(email, pwd, dbUser);
        mysql_check_pwd_ms = gateauthsupport::NowMs() - mysql_start_ms;
        if (pwd_valid)
        {
            userInfo.name = dbUser.name;
            userInfo.pwd = dbUser.password;
            userInfo.uid = dbUser.uid;
            userInfo.user_id = dbUser.user_id;
            userInfo.email = dbUser.email;
            userInfo.nick = dbUser.nick;
            userInfo.icon = dbUser.icon;
            userInfo.desc = dbUser.desc;
            userInfo.sex = dbUser.sex;
            gateauthsupport::CacheLoginProfile(email, userInfo);
        }
    }
    if (!pwd_valid)
    {
        memolog::LogWarn("gate.user_login.failed",
                         "password invalid",
                         {{"email", email}, {"error_code", std::to_string(ErrorCodes::PasswdInvalid)}});
        root["error"] = ErrorCodes::PasswdInvalid;
        WriteJson(response, root);
        return true;
    }

    std::vector<std::string> server_load_snapshot;
    std::vector<std::string> least_loaded_servers;
    std::string route_source;
    std::string status_route_detail;
    std::string http_token;
    const auto route_start_ms = gateauthsupport::NowMs();
    const auto route_nodes = gateauthsupport::SelectChatRouteForLogin(userInfo.uid,
                                                                      &server_load_snapshot,
                                                                      &least_loaded_servers,
                                                                      &route_source,
                                                                      &status_route_detail,
                                                                      &http_token);
    const auto route_select_ms = gateauthsupport::NowMs() - route_start_ms;
    if (route_nodes.empty())
    {
        memolog::LogWarn(
            "gate.user_login.failed",
            "no chat server available",
            {{"uid", std::to_string(userInfo.uid)}, {"error_code", std::to_string(ErrorCodes::RPCFailed)}});
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }

    const auto ticket_start_ms = gateauthsupport::NowMs();
    if (http_token.empty() && (!AuthCache::Instance().GetHttpToken(userInfo.uid, http_token) || http_token.empty()))
    {
        http_token = boost::uuids::to_string(boost::uuids::random_generator()());
        AuthCache::Instance().SetHttpToken(userInfo.uid, http_token);
    }
    memochat::auth::ChatLoginTicketClaims claims;
    claims.uid = userInfo.uid;
    claims.user_id = userInfo.user_id;
    claims.name = userInfo.name;
    claims.nick = userInfo.nick;
    claims.icon = userInfo.icon;
    claims.desc = userInfo.desc;
    claims.email = userInfo.email;
    claims.sex = userInfo.sex;
    claims.target_server = route_nodes.front().name;
    claims.protocol_version = gateauthsupport::LoginProtocolVersion();
    claims.issued_at_ms = gateauthsupport::NowMs();
    claims.expire_at_ms = claims.issued_at_ms + static_cast<int64_t>(gateauthsupport::GetChatTicketTtlSec()) * 1000;
    const std::string login_ticket = memochat::auth::EncodeTicket(claims, gateauthsupport::GetChatAuthSecret());
    const auto ticket_issue_ms = gateauthsupport::NowMs() - ticket_start_ms;
    if (login_ticket.empty())
    {
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }

    memolog::TraceContext::SetUid(std::to_string(userInfo.uid));
    root["error"] = 0;
    root["protocol_version"] = gateauthsupport::LoginProtocolVersion();
    root["preferred_transport"] =
        (!route_nodes.front().quic_host.empty() && !route_nodes.front().quic_port.empty()) ? "quic" : "tcp";
    root["fallback_transport"] = "tcp";
    root["email"] = email;
    root["uid"] = userInfo.uid;
    root["user_id"] = userInfo.user_id;
    root["token"] = http_token;
    root["host"] = route_nodes.front().host;
    root["port"] = route_nodes.front().port;
    root["login_ticket"] = login_ticket;
    root["ticket_expire_ms"] = static_cast<int64_t>(claims.expire_at_ms);
    memochat::json::JsonValue user_profile(memochat::json::object_t{});
    user_profile["uid"] = userInfo.uid;
    user_profile["user_id"] = userInfo.user_id;
    user_profile["name"] = userInfo.name;
    user_profile["nick"] = userInfo.nick;
    user_profile["icon"] = userInfo.icon;
    user_profile["desc"] = userInfo.desc;
    user_profile["email"] = userInfo.email;
    user_profile["sex"] = userInfo.sex;
    root["user_profile"] = user_profile;

    memochat::json::JsonValue chat_endpoints_arr(memochat::json::array_t{});
    for (const auto& route_node : route_nodes)
    {
        if (!route_node.quic_host.empty() && !route_node.quic_port.empty())
        {
            memochat::json::JsonValue quic_endpoint;
            quic_endpoint["transport"] = "quic";
            quic_endpoint["host"] = route_node.quic_host;
            quic_endpoint["port"] = route_node.quic_port;
            quic_endpoint["server_name"] = route_node.name;
            quic_endpoint["priority"] = route_node.priority;
            memochat::json::glaze_array_append(chat_endpoints_arr, quic_endpoint);
        }
        memochat::json::JsonValue endpoint;
        endpoint["transport"] = "tcp";
        endpoint["host"] = route_node.host;
        endpoint["port"] = route_node.port;
        endpoint["server_name"] = route_node.name;
        endpoint["priority"] = route_node.priority;
        memochat::json::glaze_array_append(chat_endpoints_arr, endpoint);
    }
    root["chat_endpoints"] = chat_endpoints_arr;

    memochat::json::JsonValue stage_metrics(memochat::json::object_t{});
    stage_metrics["mysql_check_pwd_ms"] = static_cast<int64_t>(mysql_check_pwd_ms);
    stage_metrics["route_select_ms"] = static_cast<int64_t>(route_select_ms);
    stage_metrics["ticket_issue_ms"] = static_cast<int64_t>(ticket_issue_ms);
    stage_metrics["user_login_total_ms"] = static_cast<int64_t>(gateauthsupport::NowMs() - login_start_ms);
    stage_metrics["route_source"] = route_source;
    stage_metrics["status_route_detail"] = status_route_detail;
    root["stage_metrics"] = stage_metrics;
    memolog::LogInfo("gate.user_login",
                     "user login succeeded",
                     {{"uid", std::to_string(userInfo.uid)},
                      {"route", "/user_login"},
                      {"chat_host", route_nodes.front().host},
                      {"chat_port", route_nodes.front().port},
                      {"chat_server", route_nodes.front().name},
                      {"route_source", route_source},
                      {"status_route_detail", status_route_detail},
                      {"login_cache_hit", login_cache_hit ? "true" : "false"},
                      {"mysql_check_pwd_ms", std::to_string(mysql_check_pwd_ms)},
                      {"route_select_ms", std::to_string(route_select_ms)},
                      {"ticket_issue_ms", std::to_string(ticket_issue_ms)},
                      {"user_login_total_ms", std::to_string(gateauthsupport::NowMs() - login_start_ms)},
                      {"server_loads",
                       [&server_load_snapshot]()
                       {
                           std::ostringstream oss;
                           for (size_t i = 0; i < server_load_snapshot.size(); ++i)
                           {
                               if (i > 0)
                               {
                                   oss << ",";
                               }
                               oss << server_load_snapshot[i];
                           }
                           return oss.str();
                       }()}});
    memolog::LogInfo("login.stage.summary",
                     "gate login stage summary",
                     {{"uid", std::to_string(userInfo.uid)},
                      {"login_cache_hit", login_cache_hit ? "true" : "false"},
                      {"mysql_check_pwd_ms", std::to_string(mysql_check_pwd_ms)},
                      {"route_select_ms", std::to_string(route_select_ms)},
                      {"ticket_issue_ms", std::to_string(ticket_issue_ms)},
                      {"user_login_total_ms", std::to_string(gateauthsupport::NowMs() - login_start_ms)}});
    const auto audit_uid = userInfo.uid;
    const auto audit_user_id = userInfo.user_id;
    const auto audit_email = email;
    const auto audit_chat_server = route_nodes.front().name;
    const auto audit_chat_host = route_nodes.front().host;
    const auto audit_chat_port = route_nodes.front().port;
    GateWorkerPool::GetInstance()->post(
        [audit_uid, audit_user_id, audit_email, audit_chat_server, audit_chat_host, audit_chat_port, login_cache_hit]()
        {
            GateAsyncSideEffects::Instance().PublishAuditLogin(audit_uid,
                                                               audit_user_id,
                                                               audit_email,
                                                               audit_chat_server,
                                                               audit_chat_host,
                                                               audit_chat_port,
                                                               login_cache_hit);
        });
    WriteJson(response, root);
    return true;
}

} // namespace memochat::gate::services::auth
