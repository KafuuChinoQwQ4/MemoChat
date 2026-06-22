#include "services/auth/AuthService.h"

#include "AuthCache.h"
#include "AuthLoginSupport.h"
#include "AuthPublicDtos.h"
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
    gateauthsupport::AuthEmailRequestDto get_varify_code_request;
    if (!gateauthsupport::DecodeAuthEmailRequest(request.body, &get_varify_code_request, &src_root))
    {
        memolog::LogWarn("gate.get_varifycode.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    if (!gateauthsupport::HasAuthEmailRequiredFields(src_root))
    {
        memolog::LogWarn("gate.get_varifycode.invalid_body", "email is missing");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = get_varify_code_request.email;
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
    gateauthsupport::AuthResetPasswordRequestDto reset_pwd_request;
    if (!gateauthsupport::DecodeAuthResetPasswordRequest(request.body, &reset_pwd_request))
    {
        memolog::LogWarn("gate.reset_pwd.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = reset_pwd_request.email;
    const auto name = reset_pwd_request.user;
    const auto pwd = reset_pwd_request.passwd;

    std::string varify_code;
    if (!AuthCache::Instance().GetVerificationCode(email, varify_code))
    {
        memolog::LogWarn("gate.reset_pwd.failed", "verify code expired", {{"email", email}});
        root["error"] = ErrorCodes::VarifyExpired;
        WriteJson(response, root);
        return true;
    }

    if (varify_code != reset_pwd_request.varifycode)
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
    gateauthsupport::AuthResetPasswordResponseDto reset_response;
    reset_response.error = 0;
    reset_response.email = email;
    reset_response.user = name;
    reset_response.passwd = pwd;
    reset_response.varifycode = reset_pwd_request.varifycode;
    root = gateauthsupport::AuthResetPasswordResponseToJsonValue(reset_response);
    WriteJson(response, root);
    return true;
}

bool AuthService::HandleUserRegister(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    gateauthsupport::AuthRegisterRequestDto register_request;
    if (!gateauthsupport::DecodeAuthRegisterRequest(request.body, &register_request))
    {
        memolog::LogWarn("gate.user_register.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = register_request.email;
    const auto name = register_request.user;
    const auto pwd = register_request.passwd;
    const auto confirm = register_request.confirm;
    const auto icon = register_request.icon;

    if (pwd != confirm)
    {
        memolog::LogWarn("gate.user_register.failed", "password mismatch", {{"email", email}});
        root["error"] = ErrorCodes::PasswdErr;
        WriteJson(response, root);
        return true;
    }

    std::string varify_code;
    if (!AuthCache::Instance().GetVerificationCode(email, varify_code))
    {
        memolog::LogWarn("gate.user_register.failed", "verify code expired", {{"email", email}});
        root["error"] = ErrorCodes::VarifyExpired;
        WriteJson(response, root);
        return true;
    }

    if (varify_code != register_request.varifycode)
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

    const std::string user_id = account_persistence.GetUserPublicId(uid);
    gateauthsupport::AuthRegisterResponseDto register_response;
    register_response.error = 0;
    register_response.uid = uid;
    register_response.user_id = user_id;
    register_response.email = email;
    register_response.user = name;
    register_response.passwd = pwd;
    register_response.confirm = confirm;
    register_response.icon = icon;
    register_response.varifycode = register_request.varifycode;
    root = gateauthsupport::AuthRegisterResponseToJsonValue(register_response);
    gateauthsupport::UserInfo cached_user;
    cached_user.uid = uid;
    cached_user.user_id = user_id;
    cached_user.name = name;
    cached_user.email = email;
    cached_user.pwd = pwd;
    cached_user.nick = name;
    cached_user.icon = icon;
    cached_user.desc = "";
    cached_user.sex = register_request.sex;
    gateauthsupport::CacheLoginProfile(email, cached_user);
    GateAsyncSideEffects::Instance().PublishUserProfileChanged(uid, user_id, email, name, name, icon, cached_user.sex);
    memolog::LogInfo("gate.user_register", "user registered", {{"email", email}, {"uid", std::to_string(uid)}});
    WriteJson(response, root);
    return true;
}

bool AuthService::HandleUserLogin(const memochat::gate::routing::GateRequest& request,
                                  memochat::gate::routing::GateResponse& response)
{
    const auto login_start_ms = gateauthsupport::NowMs();
    memochat::json::JsonValue root;
    root["trace_id"] = request.trace_id;
    gateauthsupport::AuthLoginRequestDto login_request;
    if (!gateauthsupport::DecodeAuthLoginRequest(request.body, &login_request))
    {
        memolog::LogWarn("gate.user_login.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto email = login_request.email;
    const auto pwd = login_request.passwd;
    const auto client_ver = login_request.client_ver;
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
    root["login_ticket"] = login_ticket;
    root["ticket_expire_ms"] = static_cast<int64_t>(claims.expire_at_ms);
    gateauthsupport::AuthLoginUserProfileDto user_profile_dto;
    user_profile_dto.uid = userInfo.uid;
    user_profile_dto.user_id = userInfo.user_id;
    user_profile_dto.name = userInfo.name;
    user_profile_dto.nick = userInfo.nick;
    user_profile_dto.icon = userInfo.icon;
    user_profile_dto.desc = userInfo.desc;
    user_profile_dto.email = userInfo.email;
    user_profile_dto.sex = userInfo.sex;
    root["user_profile"] = gateauthsupport::AuthLoginUserProfileToJsonValue(user_profile_dto);

    memochat::json::JsonValue chat_endpoints_arr(memochat::json::array_t{});
    for (const auto& route_node : route_nodes)
    {
        if (!route_node.quic_host.empty() && !route_node.quic_port.empty())
        {
            memochat::json::glaze_array_append(
                chat_endpoints_arr,
                gateauthsupport::AuthChatEndpointToJsonValue(
                    gateauthsupport::AuthChatEndpointDto{.transport = "quic",
                                                         .host = route_node.quic_host,
                                                         .port = route_node.quic_port,
                                                         .server_name = route_node.name,
                                                         .priority = route_node.priority}));
        }
        memochat::json::glaze_array_append(chat_endpoints_arr,
                                           gateauthsupport::AuthChatEndpointToJsonValue(
                                               gateauthsupport::AuthChatEndpointDto{.transport = "tcp",
                                                                                    .host = route_node.host,
                                                                                    .port = route_node.port,
                                                                                    .server_name = route_node.name,
                                                                                    .priority = route_node.priority}));
    }
    root["chat_endpoints"] = chat_endpoints_arr;

    gateauthsupport::AuthLoginStageMetricsDto stage_metrics_dto;
    stage_metrics_dto.mysql_check_pwd_ms = static_cast<int64_t>(mysql_check_pwd_ms);
    stage_metrics_dto.route_select_ms = static_cast<int64_t>(route_select_ms);
    stage_metrics_dto.ticket_issue_ms = static_cast<int64_t>(ticket_issue_ms);
    stage_metrics_dto.user_login_total_ms = static_cast<int64_t>(gateauthsupport::NowMs() - login_start_ms);
    stage_metrics_dto.route_source = route_source;
    stage_metrics_dto.status_route_detail = status_route_detail;
    root["stage_metrics"] = gateauthsupport::AuthLoginStageMetricsToJsonValue(stage_metrics_dto);
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
