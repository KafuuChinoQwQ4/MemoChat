#include "services/auth/AuthService.hpp"

#include "AuthCache.hpp"
#include "AuthLoginRateLimiter.hpp"
#include "AuthLoginSupport.hpp"
#include "AuthPublicDtos.hpp"
#include "AuthVerifyClient.hpp"
#include "GateAsyncSideEffects.hpp"
#include "GateWorkerPool.hpp"
#include "auth/ChatLoginTicket.hpp"
#include "auth/JwtAccessToken.hpp"
#include "auth/RefreshToken.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"
#include "random/Uuid.hpp"
#include "services/account/AccountPersistence.hpp"
#include "support/BearerAccessAuth.hpp"
#include "support/UserTokenValidator.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

#include "json/GlazeCompat.hpp"

import memochat.account.auth_service_algorithms;

namespace memochat::gate::services::auth
{

namespace auth_algo = memochat::account::auth_service::modules;
namespace account = memochat::gate::services::account;
using memochat::gate::core::AuthCache;

namespace
{

class CredentialMutationGuard
{
public:
    CredentialMutationGuard() = default;

    ~CredentialMutationGuard()
    {
        if (_uid > 0 && !_identifier.empty() && !AuthCache::Instance().ReleaseCredentialMutationLock(_uid, _identifier))
        {
            memolog::LogWarn("gate.auth.credential_lock_release_failed",
                             "credential mutation lock release failed; TTL will recover it",
                             {{"uid", std::to_string(_uid)}});
        }
    }

    CredentialMutationGuard(const CredentialMutationGuard&) = delete;
    CredentialMutationGuard& operator=(const CredentialMutationGuard&) = delete;

    bool Acquire(int uid)
    {
        if (_uid > 0 || !AuthCache::Instance().TryAcquireCredentialMutationLock(uid, _identifier))
        {
            return false;
        }
        _uid = uid;
        return true;
    }

    [[nodiscard]] bool Locked() const noexcept
    {
        return _uid > 0 && !_identifier.empty();
    }

private:
    int _uid = 0;
    std::string _identifier;
};

void WriteJson(memochat::gate::routing::GateResponse& response, const memochat::json::JsonValue& root)
{
    response.status = auth_algo::ResponseOkStatus();
    response.content_type = auth_algo::JsonContentType();
    response.body = root.toStyledString();
}

void WriteRateLimited(memochat::gate::routing::GateResponse& response,
                      memochat::json::JsonValue& root,
                      const gateauthsupport::AuthLoginRateLimitResult& rate_limit)
{
    root["error"] = ErrorCodes::RateLimited;
    root["rate_limited"] = true;
    root["retry_after_sec"] = rate_limit.retry_after_sec;
    WriteJson(response, root);
}

bool WriteValidationFailed(memochat::gate::routing::GateResponse& response,
                           memochat::json::JsonValue& root,
                           const char* route,
                           const gateauthsupport::AuthInputValidationResult& validation)
{
    root["error"] = validation.code == gateauthsupport::AuthInputValidationCode::InvalidEmail ? ErrorCodes::InvalidEmail
                                                                                              : ErrorCodes::Error_Json;
    memolog::LogWarn("gate.auth.invalid_input",
                     "auth input rejected before side effects",
                     {{"route", route},
                      {"field", gateauthsupport::AuthInputFieldName(validation.field)},
                      {"reason", gateauthsupport::AuthInputValidationCodeName(validation.code)},
                      {"max_length", std::to_string(validation.max_length)}});
    WriteJson(response, root);
    return true;
}

bool WriteRequestRateLimitFailure(memochat::gate::routing::GateResponse& response,
                                  memochat::json::JsonValue& root,
                                  const char* route,
                                  const gateauthsupport::AuthLoginRateLimitResult& rate_limit)
{
    if (rate_limit.redis_error)
    {
        memolog::LogWarn("gate.auth.request_rate_limit_redis_error",
                         "auth request rate-limit counter failed closed",
                         {{"route", route}});
        root["error"] = ErrorCodes::RateLimited;
        root["rate_limited"] = true;
        root["retry_after_sec"] = 60;
        WriteJson(response, root);
        return true;
    }
    if (rate_limit.rate_limited)
    {
        memolog::LogWarn("gate.auth.request_rate_limited",
                         "too many auth requests",
                         {{"route", route}, {"retry_after_sec", std::to_string(rate_limit.retry_after_sec)}});
        WriteRateLimited(response, root, rate_limit);
        return true;
    }
    return false;
}

bool EnforceAuthRequestRateLimit(memochat::gate::routing::GateResponse& response,
                                 memochat::json::JsonValue& root,
                                 const memochat::gate::routing::GateRequest& request,
                                 gateauthsupport::AuthRateLimitAction action,
                                 std::string_view subject,
                                 const char* route)
{
    const auto rate_limit = gateauthsupport::CheckAuthRequestRateLimit(action, subject, request);
    return WriteRequestRateLimitFailure(response, root, route, rate_limit);
}

bool InvalidatePasswordResetRedisState(const std::string& email, int uid)
{
    const bool token_deleted = AuthCache::Instance().DeleteHttpToken(uid);
    const bool email_profile_deleted = gateauthsupport::InvalidateLoginCacheByEmail(email);
    const bool uid_profile_deleted = gateauthsupport::InvalidateLoginCacheByUid(uid);
    return token_deleted && email_profile_deleted && uid_profile_deleted;
}

std::string Trim(std::string value)
{
    auto not_space = [](unsigned char ch)
    {
        return !std::isspace(ch);
    };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string LowerAscii(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

std::string HeaderValue(const memochat::gate::routing::GateRequest& request, const std::string& name)
{
    const auto expected = LowerAscii(name);
    for (const auto& [key, value] : request.headers)
    {
        if (LowerAscii(key) == expected)
        {
            return value;
        }
    }
    return "";
}

std::string RefreshTokenUserAgent(const memochat::gate::routing::GateRequest& request)
{
    return Trim(HeaderValue(request, "user-agent"));
}

std::string RefreshTokenIpHash(const memochat::gate::routing::GateRequest& request)
{
    const auto client_ip = gateauthsupport::ResolveLoginRateLimitClientIp(request);
    std::string ip_hash;
    if (!memochat::auth::HashRefreshTokenMetadata(client_ip, ip_hash))
    {
        return "";
    }
    return ip_hash;
}

gateauthsupport::UserInfo ToAuthUserInfo(const account::AccountProfile& profile)
{
    gateauthsupport::UserInfo userInfo;
    userInfo.name = profile.name;
    userInfo.uid = profile.uid;
    userInfo.user_id = profile.user_id;
    userInfo.email = profile.email;
    userInfo.nick = profile.nick;
    userInfo.icon = profile.icon;
    userInfo.desc = profile.desc;
    userInfo.sex = profile.sex;
    return userInfo;
}

bool AttachRefreshTokenForClient(memochat::json::JsonValue& root,
                                 memochat::gate::routing::GateResponse& response,
                                 const std::string& refresh_token,
                                 int refresh_ttl_sec,
                                 bool expose_in_json)
{
    if (expose_in_json)
    {
        root["refresh_token"] = refresh_token;
        root["refresh_token_expires_in_sec"] = refresh_ttl_sec;
        return true;
    }

    const auto cookie = gateauthsupport::BuildWebRefreshTokenCookie(refresh_token, refresh_ttl_sec);
    if (cookie.empty())
    {
        return false;
    }
    response.headers["Set-Cookie"] = cookie;
    return true;
}

bool AttachIssuedRefreshToken(memochat::json::JsonValue& root,
                              memochat::gate::routing::GateResponse& response,
                              int uid,
                              const memochat::gate::routing::GateRequest& request,
                              bool expose_in_json)
{
    const int refresh_ttl_sec = gateauthsupport::GetRefreshTokenTtlSec();
    const auto refresh_token = account::AccountPersistence::Instance().IssueRefreshToken(uid,
                                                                                         refresh_ttl_sec,
                                                                                         RefreshTokenUserAgent(request),
                                                                                         RefreshTokenIpHash(request));
    if (!refresh_token.ok)
    {
        memolog::LogError("gate.auth.refresh_issue_failed",
                          "refresh token issue failed",
                          {{"uid", std::to_string(uid)}});
        root["error"] = ErrorCodes::RPCFailed;
        return false;
    }

    return AttachRefreshTokenForClient(root, response, refresh_token.refresh_token, refresh_ttl_sec, expose_in_json);
}

bool IssueLoginSessionForUser(const memochat::gate::routing::GateRequest& request,
                              const gateauthsupport::UserInfo& userInfo,
                              memochat::json::JsonValue& root,
                              int64_t session_start_ms,
                              int64_t account_check_ms,
                              bool login_cache_hit)
{
    std::vector<std::string> server_load_snapshot;
    std::vector<std::string> least_loaded_servers;
    std::string route_source;
    std::string status_route_detail;
    std::string access_token;
    const auto route_start_ms = gateauthsupport::NowMs();
    const auto route_nodes = gateauthsupport::SelectChatRouteForLogin(userInfo.uid,
                                                                      &server_load_snapshot,
                                                                      &least_loaded_servers,
                                                                      &route_source,
                                                                      &status_route_detail,
                                                                      &access_token);
    const auto route_select_ms = gateauthsupport::NowMs() - route_start_ms;
    if (route_nodes.empty())
    {
        memolog::LogWarn(
            "gate.auth.session_issue_failed",
            "no chat server available",
            {{"uid", std::to_string(userInfo.uid)}, {"error_code", std::to_string(ErrorCodes::RPCFailed)}});
        root["error"] = ErrorCodes::RPCFailed;
        return false;
    }

    const auto ticket_start_ms = gateauthsupport::NowMs();
    const int access_token_ttl_sec = gateauthsupport::GetAccessTokenTtlSec();
    const int64_t access_token_now_sec = gateauthsupport::NowMs() / 1000;
    std::string access_jti;
    std::string ticket_jti;
    std::string uuid_error;
    if (!memochat::random::GenerateUuid(access_jti, &uuid_error) ||
        !memochat::random::GenerateUuid(ticket_jti, &uuid_error))
    {
        memolog::LogError("gate.auth.session_issue_failed",
                          "session identifier generation failed",
                          {{"uid", std::to_string(userInfo.uid)}, {"error", uuid_error}});
        root["error"] = ErrorCodes::RPCFailed;
        return false;
    }
    memochat::auth::JwtAccessTokenClaims access_claims;
    access_claims.typ = "access";
    access_claims.iss = gateauthsupport::GetJwtAccessIssuer();
    access_claims.aud = gateauthsupport::GetJwtAccessAudience();
    access_claims.sub = std::to_string(userInfo.uid);
    access_claims.uid = userInfo.uid;
    access_claims.iat = access_token_now_sec;
    access_claims.nbf = access_token_now_sec;
    access_claims.exp = access_token_now_sec + access_token_ttl_sec;
    access_claims.jti = std::move(access_jti);
    access_claims.token_version = 0;
    access_token = memochat::auth::EncodeAccessToken(access_claims, gateauthsupport::GetJwtAccessSecret());
    if (access_token.empty() || !AuthCache::Instance().SetHttpToken(userInfo.uid, access_token, access_token_ttl_sec))
    {
        root["error"] = ErrorCodes::RPCFailed;
        return false;
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
    claims.jti = std::move(ticket_jti);
    const std::string login_ticket = memochat::auth::EncodeTicket(claims, gateauthsupport::GetChatAuthSecret());
    const auto ticket_issue_ms = gateauthsupport::NowMs() - ticket_start_ms;
    if (login_ticket.empty())
    {
        root["error"] = ErrorCodes::RPCFailed;
        return false;
    }

    memolog::TraceContext::SetUid(std::to_string(userInfo.uid));
    root["error"] = 0;
    root["protocol_version"] = gateauthsupport::LoginProtocolVersion();
    root["preferred_transport"] =
        auth_algo::PreferredTransport(!route_nodes.front().quic_host.empty(), !route_nodes.front().quic_port.empty());
    root["fallback_transport"] = auth_algo::FallbackTransport();
    root["email"] = userInfo.email;
    root["uid"] = userInfo.uid;
    root["user_id"] = userInfo.user_id;
    root["access_token"] = access_token;
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
        if (route_node.ws_enabled && !route_node.ws_host.empty() && !route_node.ws_port.empty())
        {
            memochat::json::glaze_array_append(
                chat_endpoints_arr,
                gateauthsupport::AuthChatEndpointToJsonValue(gateauthsupport::AuthChatEndpointDto{
                    .transport = auth_algo::WebSocketTransport(),
                    .host = route_node.ws_host,
                    .port = route_node.ws_port,
                    .path = route_node.ws_path.empty() ? "/ws" : route_node.ws_path,
                    .tls = route_node.ws_tls,
                    .server_name = route_node.name,
                    .priority = route_node.priority}));
        }
        if (route_node.wt_enabled && !route_node.wt_host.empty() && !route_node.wt_port.empty())
        {
            memochat::json::glaze_array_append(
                chat_endpoints_arr,
                gateauthsupport::AuthChatEndpointToJsonValue(gateauthsupport::AuthChatEndpointDto{
                    .transport = auth_algo::WebTransportTransport(),
                    .host = route_node.wt_host,
                    .port = route_node.wt_port,
                    .path = route_node.wt_path.empty() ? "/chat" : route_node.wt_path,
                    .tls = route_node.wt_tls,
                    .server_name = route_node.name,
                    .priority = route_node.priority}));
        }
        if (!route_node.quic_host.empty() && !route_node.quic_port.empty())
        {
            memochat::json::glaze_array_append(
                chat_endpoints_arr,
                gateauthsupport::AuthChatEndpointToJsonValue(
                    gateauthsupport::AuthChatEndpointDto{.transport = auth_algo::QuicTransport(),
                                                         .host = route_node.quic_host,
                                                         .port = route_node.quic_port,
                                                         .path = "",
                                                         .tls = false,
                                                         .server_name = route_node.name,
                                                         .priority = route_node.priority}));
        }
        memochat::json::glaze_array_append(
            chat_endpoints_arr,
            gateauthsupport::AuthChatEndpointToJsonValue(
                gateauthsupport::AuthChatEndpointDto{.transport = auth_algo::TcpTransport(),
                                                     .host = route_node.host,
                                                     .port = route_node.port,
                                                     .path = "",
                                                     .tls = false,
                                                     .server_name = route_node.name,
                                                     .priority = route_node.priority}));
    }
    root["chat_endpoints"] = chat_endpoints_arr;

    gateauthsupport::AuthLoginStageMetricsDto stage_metrics_dto;
    stage_metrics_dto.mysql_check_pwd_ms = static_cast<int64_t>(account_check_ms);
    stage_metrics_dto.route_select_ms = static_cast<int64_t>(route_select_ms);
    stage_metrics_dto.ticket_issue_ms = static_cast<int64_t>(ticket_issue_ms);
    stage_metrics_dto.user_login_total_ms = static_cast<int64_t>(gateauthsupport::NowMs() - session_start_ms);
    stage_metrics_dto.route_source = route_source;
    stage_metrics_dto.status_route_detail = status_route_detail;
    root["stage_metrics"] = gateauthsupport::AuthLoginStageMetricsToJsonValue(stage_metrics_dto);
    memolog::LogInfo("gate.auth.session_issued",
                     "auth session issued",
                     {{"uid", std::to_string(userInfo.uid)},
                      {"route", request.path.empty() ? "/user_login" : request.path},
                      {"chat_host", route_nodes.front().host},
                      {"chat_port", route_nodes.front().port},
                      {"chat_server", route_nodes.front().name},
                      {"route_source", route_source},
                      {"status_route_detail", status_route_detail},
                      {"login_cache_hit", login_cache_hit ? "true" : "false"},
                      {"account_check_ms", std::to_string(account_check_ms)},
                      {"route_select_ms", std::to_string(route_select_ms)},
                      {"ticket_issue_ms", std::to_string(ticket_issue_ms)},
                      {"session_issue_total_ms", std::to_string(gateauthsupport::NowMs() - session_start_ms)},
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
                      {"mysql_check_pwd_ms", std::to_string(account_check_ms)},
                      {"route_select_ms", std::to_string(route_select_ms)},
                      {"ticket_issue_ms", std::to_string(ticket_issue_ms)},
                      {"user_login_total_ms", std::to_string(gateauthsupport::NowMs() - session_start_ms)}});
    const auto audit_uid = userInfo.uid;
    const auto audit_user_id = userInfo.user_id;
    const auto audit_email = userInfo.email;
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
    return true;
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

    if (const auto validation = gateauthsupport::ValidateAuthEmailRequest(get_varify_code_request); !validation.ok())
    {
        return WriteValidationFailed(response, root, "/get_varifycode", validation);
    }
    const auto& email = get_varify_code_request.email;
    if (EnforceAuthRequestRateLimit(response,
                                    root,
                                    request,
                                    gateauthsupport::AuthRateLimitAction::GetVarifyCode,
                                    email,
                                    "/get_varifycode"))
    {
        return true;
    }

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

    if (const auto validation = gateauthsupport::ValidateAuthResetPasswordRequest(reset_pwd_request); !validation.ok())
    {
        return WriteValidationFailed(response, root, "/reset_pwd", validation);
    }
    const auto& email = reset_pwd_request.email;
    const auto& name = reset_pwd_request.user;
    const auto& pwd = reset_pwd_request.passwd;
    if (EnforceAuthRequestRateLimit(response,
                                    root,
                                    request,
                                    gateauthsupport::AuthRateLimitAction::ResetPassword,
                                    email,
                                    "/reset_pwd"))
    {
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

    int reset_uid = 0;
    std::string reset_name;
    if (!account_persistence.FindUserByEmail(email, reset_uid, reset_name) || reset_uid <= 0)
    {
        memolog::LogWarn("gate.reset_pwd.failed", "user lookup failed", {{"email", email}});
        root["error"] = ErrorCodes::EmailNotMatch;
        WriteJson(response, root);
        return true;
    }

    CredentialMutationGuard credential_guard;
    if (!credential_guard.Acquire(reset_uid))
    {
        memolog::LogWarn("gate.reset_pwd.failed",
                         "credential mutation is already in progress",
                         {{"email", email}, {"uid", std::to_string(reset_uid)}});
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }

    std::string varify_code;
    if (!AuthCache::Instance().GetVerificationCode(email, varify_code))
    {
        memolog::LogWarn("gate.reset_pwd.failed", "verify code expired", {{"email", email}});
        root["error"] = ErrorCodes::VarifyExpired;
        WriteJson(response, root);
        return true;
    }
    if (varify_code != reset_pwd_request.varifycode ||
        !AuthCache::Instance().ConsumeVerificationCode(email, reset_pwd_request.varifycode))
    {
        memolog::LogWarn("gate.reset_pwd.failed", "verify code mismatch or already consumed", {{"email", email}});
        root["error"] = ErrorCodes::VarifyCodeErr;
        WriteJson(response, root);
        return true;
    }

    // Revoke the authoritative Redis token binding before committing the new
    // password. If Redis is unavailable, the password remains unchanged and an
    // old access token cannot reappear as valid when Redis recovers.
    if (!InvalidatePasswordResetRedisState(email, reset_uid))
    {
        memolog::LogError("gate.reset_pwd.failed",
                          "credential revocation failed before password update",
                          {{"email", email}, {"uid", std::to_string(reset_uid)}});
        root["error"] = ErrorCodes::RPCFailed;
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

    // Repeat after the Postgres transaction to cover a token issued just before
    // the password change committed. UpdatePwd revokes every refresh token in
    // that same transaction.
    if (!InvalidatePasswordResetRedisState(email, reset_uid))
    {
        memolog::LogError("gate.reset_pwd.failed",
                          "credential revocation failed after password update",
                          {{"email", email}, {"uid", std::to_string(reset_uid)}});
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }
    memolog::LogInfo("gate.reset_pwd", "password updated and credentials revoked", {{"email", email}});
    gateauthsupport::AuthResetPasswordResponseDto reset_response;
    reset_response.error = 0;
    reset_response.email = email;
    reset_response.user = name;
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

    if (const auto validation = gateauthsupport::ValidateAuthRegisterRequest(register_request); !validation.ok())
    {
        return WriteValidationFailed(response, root, "/user_register", validation);
    }
    const auto& email = register_request.email;
    const auto& name = register_request.user;
    const auto& pwd = register_request.passwd;
    const auto& confirm = register_request.confirm;
    const auto& icon = register_request.icon;
    if (EnforceAuthRequestRateLimit(response,
                                    root,
                                    request,
                                    gateauthsupport::AuthRateLimitAction::Register,
                                    email,
                                    "/user_register"))
    {
        return true;
    }

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
    if (!auth_algo::IsRegisteredUidValid(uid))
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
    register_response.icon = icon;
    register_response.varifycode = register_request.varifycode;
    root = gateauthsupport::AuthRegisterResponseToJsonValue(register_response);
    gateauthsupport::UserInfo cached_user;
    cached_user.uid = uid;
    cached_user.user_id = user_id;
    cached_user.name = name;
    cached_user.email = email;
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

    if (const auto validation = gateauthsupport::ValidateAuthLoginRequest(login_request); !validation.ok())
    {
        return WriteValidationFailed(response, root, "/user_login", validation);
    }
    const auto& email = login_request.email;
    const auto& pwd = login_request.passwd;
    const auto& client_ver = login_request.client_ver;
    root["min_version"] = gateauthsupport::MinClientVersion();
    root["feature_group_chat"] = auth_algo::FeatureGroupChatEnabled();
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

    const auto rate_limit = gateauthsupport::CheckLoginRateLimit(email, request);
    if (rate_limit.redis_error)
    {
        memolog::LogWarn("gate.user_login.rate_limit_redis_error",
                         "login rate-limit counter check failed closed",
                         {{"email", email}});
        root["error"] = ErrorCodes::RateLimited;
        root["rate_limited"] = true;
        root["retry_after_sec"] = 60;
        WriteJson(response, root);
        return true;
    }
    if (rate_limit.rate_limited)
    {
        memolog::LogWarn("gate.user_login.rate_limited",
                         "too many failed login attempts",
                         {{"email", email}, {"retry_after_sec", std::to_string(rate_limit.retry_after_sec)}});
        WriteRateLimited(response, root, rate_limit);
        return true;
    }

    gateauthsupport::UserInfo userInfo;
    const auto mysql_start_ms = gateauthsupport::NowMs();
    gateauthsupport::UserInfo tempUser;
    bool login_cache_hit = gateauthsupport::TryLoadCachedLoginProfile(email, tempUser);
    bool pwd_valid = false;
    int64_t mysql_check_pwd_ms = 0;
    account::AccountProfile dbUser;
    CredentialMutationGuard credential_guard;
    int credential_uid = 0;
    std::string credential_name;
    const bool credential_owner_found =
        account::AccountPersistence::Instance().FindUserByEmail(email, credential_uid, credential_name) &&
        credential_uid > 0;
    if (credential_owner_found && !credential_guard.Acquire(credential_uid))
    {
        memolog::LogWarn("gate.user_login.failed",
                         "credential mutation is already in progress",
                         {{"email", email}, {"uid", std::to_string(credential_uid)}});
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }
    pwd_valid = account::AccountPersistence::Instance().CheckPassword(email, pwd, dbUser);
    if (pwd_valid && (!credential_guard.Locked() || dbUser.uid != credential_uid))
    {
        memolog::LogError("gate.user_login.failed", "credential owner changed during login", {{"email", email}});
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }
    mysql_check_pwd_ms = gateauthsupport::NowMs() - mysql_start_ms;
    if (pwd_valid)
    {
        if (login_cache_hit && tempUser.uid == dbUser.uid)
        {
            userInfo = tempUser;
            gateauthsupport::RefreshLoginProfileFromDb(email, userInfo);
        }
        else
        {
            userInfo.name = dbUser.name;
            userInfo.uid = dbUser.uid;
            userInfo.user_id = dbUser.user_id;
            userInfo.email = dbUser.email;
            userInfo.nick = dbUser.nick;
            userInfo.icon = dbUser.icon;
            userInfo.desc = dbUser.desc;
            userInfo.sex = dbUser.sex;
            gateauthsupport::CacheLoginProfile(email, userInfo);
            login_cache_hit = false;
        }
    }
    if (!pwd_valid)
    {
        const auto failure_limit = gateauthsupport::RecordLoginFailure(email, request);
        if (failure_limit.redis_error)
        {
            memolog::LogWarn("gate.user_login.rate_limit_redis_error",
                             "login rate-limit failure counter update failed closed",
                             {{"email", email}});
            root["error"] = ErrorCodes::RateLimited;
            root["rate_limited"] = true;
            root["retry_after_sec"] = 60;
            WriteJson(response, root);
            return true;
        }
        if (failure_limit.rate_limited)
        {
            memolog::LogWarn("gate.user_login.rate_limited",
                             "failed login threshold reached",
                             {{"email", email}, {"retry_after_sec", std::to_string(failure_limit.retry_after_sec)}});
            WriteRateLimited(response, root, failure_limit);
            return true;
        }
        memolog::LogWarn("gate.user_login.failed",
                         "password invalid",
                         {{"email", email}, {"error_code", std::to_string(ErrorCodes::PasswdInvalid)}});
        root["error"] = ErrorCodes::PasswdInvalid;
        WriteJson(response, root);
        return true;
    }
    gateauthsupport::ClearLoginFailureCounters(email, request);

    memochat::json::JsonValue session_root = root;
    if (!IssueLoginSessionForUser(request, userInfo, session_root, login_start_ms, mysql_check_pwd_ms, login_cache_hit))
    {
        WriteJson(response, session_root);
        return true;
    }
    const auto client_marker = HeaderValue(request, "x-memochat-client");
    const bool expose_refresh_token = auth_algo::ShouldIssueRefreshToken(client_marker.data(), client_marker.size());
    if (!AttachIssuedRefreshToken(session_root, response, userInfo.uid, request, expose_refresh_token))
    {
        memochat::json::JsonValue failure_root;
        failure_root["trace_id"] = request.trace_id;
        failure_root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, failure_root);
        return true;
    }

    memolog::LogInfo("gate.user_login", "user login succeeded", {{"uid", std::to_string(userInfo.uid)}});
    WriteJson(response, session_root);
    return true;
}

bool AuthService::HandleAuthRefresh(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    const auto refresh_start_ms = gateauthsupport::NowMs();
    memochat::json::JsonValue root;
    root["trace_id"] = request.trace_id;
    root["min_version"] = gateauthsupport::MinClientVersion();
    root["feature_group_chat"] = auth_algo::FeatureGroupChatEnabled();

    gateauthsupport::AuthRefreshRequestDto refresh_request;
    if (!gateauthsupport::DecodeAuthRefreshRequest(request.body, &refresh_request))
    {
        memolog::LogWarn("gate.auth_refresh.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto client_marker = HeaderValue(request, "x-memochat-client");
    const bool expose_refresh_token = auth_algo::ShouldIssueRefreshToken(client_marker.data(), client_marker.size());
    if (!expose_refresh_token && refresh_request.refresh_token.empty())
    {
        refresh_request.refresh_token = gateauthsupport::ExtractWebRefreshTokenCookie(HeaderValue(request, "cookie"));
    }

    if (const auto validation = gateauthsupport::ValidateAuthRefreshRequest(refresh_request); !validation.ok())
    {
        if (!expose_refresh_token)
        {
            response.headers["Set-Cookie"] = gateauthsupport::ClearWebRefreshTokenCookie();
        }
        return WriteValidationFailed(response, root, "/auth_refresh", validation);
    }
    if (!gateauthsupport::IsClientVersionAllowed(refresh_request.client_ver, gateauthsupport::MinClientVersion()))
    {
        root["error"] = ErrorCodes::ClientVersionTooLow;
        memolog::LogWarn("gate.auth_refresh.failed",
                         "client version too low",
                         {{"client_ver", refresh_request.client_ver},
                          {"error_code", std::to_string(ErrorCodes::ClientVersionTooLow)}});
        WriteJson(response, root);
        return true;
    }
    if (EnforceAuthRequestRateLimit(response,
                                    root,
                                    request,
                                    gateauthsupport::AuthRateLimitAction::AuthRefresh,
                                    gateauthsupport::AuthRefreshTokenRateLimitSubject(refresh_request.refresh_token),
                                    "/auth_refresh"))
    {
        return true;
    }

    auto& account_persistence = account::AccountPersistence::Instance();
    int refresh_uid = 0;
    if (!account_persistence.ResolveActiveRefreshTokenUserId(refresh_request.refresh_token, refresh_uid))
    {
        if (!expose_refresh_token)
        {
            response.headers["Set-Cookie"] = gateauthsupport::ClearWebRefreshTokenCookie();
        }
        root["error"] = ErrorCodes::TokenInvalid;
        memolog::LogWarn("gate.auth_refresh.failed", "refresh token is not active", {});
        WriteJson(response, root);
        return true;
    }

    CredentialMutationGuard credential_guard;
    if (!credential_guard.Acquire(refresh_uid))
    {
        root["error"] = ErrorCodes::RPCFailed;
        memolog::LogWarn("gate.auth_refresh.failed",
                         "credential mutation is already in progress",
                         {{"uid", std::to_string(refresh_uid)}});
        WriteJson(response, root);
        return true;
    }

    const auto rotated = account_persistence.RotateRefreshToken(refresh_request.refresh_token,
                                                                gateauthsupport::GetRefreshTokenTtlSec(),
                                                                RefreshTokenUserAgent(request),
                                                                RefreshTokenIpHash(request));

    if (rotated.status != account::RefreshTokenStatus::Success)
    {
        if (!expose_refresh_token)
        {
            response.headers["Set-Cookie"] = gateauthsupport::ClearWebRefreshTokenCookie();
        }
        int error_code = ErrorCodes::TokenInvalid;
        if (rotated.status == account::RefreshTokenStatus::StorageError)
        {
            error_code = ErrorCodes::RPCFailed;
        }
        root["error"] = error_code;
        memolog::LogWarn("gate.auth_refresh.failed",
                         "refresh token rejected",
                         {{"uid", std::to_string(rotated.uid)},
                          {"status", std::to_string(static_cast<int>(rotated.status))},
                          {"error_code", std::to_string(error_code)}});
        WriteJson(response, root);
        return true;
    }

    if (!account_persistence.IsRefreshTokenActiveForUid(rotated.refresh_token, rotated.uid))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        memolog::LogWarn("gate.auth_refresh.failed",
                         "rotated refresh token was revoked before access-token issue",
                         {{"uid", std::to_string(rotated.uid)}});
        WriteJson(response, root);
        return true;
    }

    auto userInfo = ToAuthUserInfo(rotated.profile);
    gateauthsupport::CacheLoginProfile(userInfo.email, userInfo);
    if (!IssueLoginSessionForUser(request, userInfo, root, refresh_start_ms, 0, false))
    {
        WriteJson(response, root);
        return true;
    }

    if (!AttachRefreshTokenForClient(root,
                                     response,
                                     rotated.refresh_token,
                                     gateauthsupport::GetRefreshTokenTtlSec(),
                                     expose_refresh_token))
    {
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }
    memolog::LogInfo("gate.auth_refresh",
                     "refresh token rotated and session issued",
                     {{"uid", std::to_string(userInfo.uid)}});
    WriteJson(response, root);
    return true;
}

bool AuthService::HandleAuthLogout(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    root["trace_id"] = request.trace_id;
    root["min_version"] = gateauthsupport::MinClientVersion();

    gateauthsupport::AuthLogoutRequestDto logout_request;
    if (!gateauthsupport::DecodeAuthLogoutRequest(request.body, &logout_request))
    {
        memolog::LogWarn("gate.auth_logout.invalid_json", "request json parse failed");
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto client_marker = HeaderValue(request, "x-memochat-client");
    const bool expose_refresh_token = auth_algo::ShouldIssueRefreshToken(client_marker.data(), client_marker.size());
    if (!expose_refresh_token)
    {
        if (logout_request.refresh_token.empty())
        {
            logout_request.refresh_token =
                gateauthsupport::ExtractWebRefreshTokenCookie(HeaderValue(request, "cookie"));
        }
        response.headers["Set-Cookie"] = gateauthsupport::ClearWebRefreshTokenCookie();
    }

    if (const auto validation = gateauthsupport::ValidateAuthLogoutRequest(logout_request); !validation.ok())
    {
        return WriteValidationFailed(response, root, "/auth_logout", validation);
    }
    if (!gateauthsupport::IsClientVersionAllowed(logout_request.client_ver, gateauthsupport::MinClientVersion()))
    {
        root["error"] = ErrorCodes::ClientVersionTooLow;
        memolog::LogWarn("gate.auth_logout.failed",
                         "client version too low",
                         {{"client_ver", logout_request.client_ver},
                          {"error_code", std::to_string(ErrorCodes::ClientVersionTooLow)}});
        WriteJson(response, root);
        return true;
    }

    int authenticated_uid = 0;
    if (!memochat::auth::ResolveBearerAccessUserId(request, authenticated_uid))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        memolog::LogWarn("gate.auth_logout.failed",
                         "bearer access token rejected",
                         {{"error_code", std::to_string(ErrorCodes::TokenInvalid)}});
        WriteJson(response, root);
        return true;
    }

    CredentialMutationGuard credential_guard;
    if (!credential_guard.Acquire(authenticated_uid))
    {
        root["error"] = ErrorCodes::RPCFailed;
        memolog::LogWarn("gate.auth_logout.failed",
                         "credential mutation is already in progress",
                         {{"uid", std::to_string(authenticated_uid)}});
        WriteJson(response, root);
        return true;
    }

    if (!AuthCache::Instance().DeleteHttpToken(authenticated_uid))
    {
        root["error"] = ErrorCodes::RPCFailed;
        memolog::LogError("gate.auth_logout.failed",
                          "access token revocation failed closed",
                          {{"uid", std::to_string(authenticated_uid)}});
        WriteJson(response, root);
        return true;
    }

    bool revoked = false;
    if (logout_request.all_devices)
    {
        revoked = account::AccountPersistence::Instance().RevokeAllRefreshTokensForUid(authenticated_uid);
    }
    else
    {
        int refresh_uid = 0;
        revoked =
            !logout_request.refresh_token.empty() &&
            account::AccountPersistence::Instance().RevokeRefreshToken(logout_request.refresh_token, refresh_uid) &&
            refresh_uid == authenticated_uid;
    }

    if (!revoked)
    {
        root["error"] = ErrorCodes::TokenInvalid;
        memolog::LogWarn("gate.auth_logout.failed",
                         "refresh token revoke failed",
                         {{"uid", std::to_string(authenticated_uid)},
                          {"all_devices", logout_request.all_devices ? "true" : "false"},
                          {"error_code", std::to_string(ErrorCodes::TokenInvalid)}});
        WriteJson(response, root);
        return true;
    }

    if (!gateauthsupport::InvalidateLoginCacheByUid(authenticated_uid))
    {
        memolog::LogWarn("gate.auth_logout.cache_invalidate_failed",
                         "access and refresh credentials were revoked but profile cache cleanup failed",
                         {{"uid", std::to_string(authenticated_uid)}});
    }

    gateauthsupport::AuthLogoutResponseDto logout_response;
    logout_response.error = ErrorCodes::Success;
    logout_response.uid = authenticated_uid;
    logout_response.all_devices = logout_request.all_devices;
    root = gateauthsupport::AuthLogoutResponseToJsonValue(logout_response);
    memolog::LogInfo(
        "gate.auth_logout",
        "auth credentials revoked",
        {{"uid", std::to_string(authenticated_uid)}, {"all_devices", logout_request.all_devices ? "true" : "false"}});
    WriteJson(response, root);
    return true;
}

} // namespace memochat::gate::services::auth
