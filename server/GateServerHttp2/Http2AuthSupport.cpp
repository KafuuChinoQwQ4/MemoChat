#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2AuthSupport.h"
#include "../GateServerCore/PostgresMgr.h"
#include "../GateServerCore/PostgresDao.h"
#include "../GateServerCore/RedisMgr.h"
#include "../GateServerCore/VerifyGrpcClient.h"
#include "../GateServerCore/GateAsyncSideEffects.h"
#include "../GateServerCore/AuthLoginSupport.h"
#include "../GateServerCore/const.h"
#include "logging/Logger.h"
#include "../common/auth/ChatLoginTicket.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>

namespace Http2AuthSupport {

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root) {
    std::string body_str(body_sv);
    return memochat::json::glaze_parse(root, body_str) && memochat::json::glaze_is_object(root);
}

memochat::json::JsonValue MakeError(int error_code, const std::string& message) {
    memochat::json::JsonValue resp;
    resp["error"] = error_code;
    if (!message.empty()) {
        resp["message"] = message;
    }
    return resp;
}

memochat::json::JsonValue MakeOk(const memochat::json::JsonValue& data) {
    memochat::json::JsonValue resp;
    resp["error"] = 0;
    if (memochat::json::glaze_is_object(data) || memochat::json::glaze_is_array(data)) {
        for (const auto& key : memochat::json::getMemberNames(data)) {
            resp[key] = data[key];
        }
    }
    return resp;
}

LoginResult HandleGetVarifyCode(const std::string& email) {
    LoginResult result;
    if (email.empty()) {
        result.error = 1;
        result.message = "email is required";
        return result;
    }
    GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
    result.error = rsp.error();
    result.message = rsp.error() == 0 ? "verification code sent" : "verification code failed";
    result.data["email"] = email;
    memolog::LogInfo("http2.get_varifycode",
        "verify code requested via HTTP2 handler",
        {{"email", email}, {"error_code", std::to_string(rsp.error())}});
    return result;
}

LoginResult HandleUserRegister(const memochat::json::JsonValue& req) {
    LoginResult result;
    if (!memochat::json::glaze_has_key(req, "email") || !memochat::json::glaze_has_key(req, "user") ||
        !memochat::json::glaze_has_key(req, "passwd") || !memochat::json::glaze_has_key(req, "confirm")) {
        result.error = 1;
        result.message = "missing required fields";
        return result;
    }

    const auto email = memochat::json::glaze_safe_get<std::string>(req, "email", "");
    const auto name = memochat::json::glaze_safe_get<std::string>(req, "user", "");
    const auto pwd = memochat::json::glaze_safe_get<std::string>(req, "passwd", "");
    const auto confirm = memochat::json::glaze_safe_get<std::string>(req, "confirm", "");
    const auto icon = memochat::json::glaze_safe_get<std::string>(req, "icon", "");

    if (pwd != confirm) {
        result.error = 1001;
        result.message = "password mismatch";
        return result;
    }

    std::string varify_code;
    if (!RedisMgr::GetInstance()->Get(CODEPREFIX + email, varify_code)) {
        result.error = 1002;
        result.message = "verification code expired";
        return result;
    }
    if (varify_code != memochat::json::glaze_safe_get<std::string>(req, "varifycode", "")) {
        result.error = 1003;
        result.message = "verification code mismatch";
        return result;
    }

    const int uid = PostgresMgr::GetInstance()->RegUser(name, email, pwd, icon);
    if (uid <= 0) {
        result.error = ErrorCodes::UserExist;
        result.message = "user or email already exists";
        return result;
    }

    result.error = ErrorCodes::Success;
    result.data["uid"] = uid;
    result.data["user_id"] = PostgresMgr::GetInstance()->GetUserPublicId(uid);
    result.data["email"] = email;
    result.data["user"] = name;

    gateauthsupport::UserInfo cached_user;
    cached_user.uid = uid;
    cached_user.user_id = memochat::json::glaze_safe_get<std::string>(result.data, "user_id", "");
    cached_user.name = name;
    cached_user.email = email;
    cached_user.pwd = pwd;
    cached_user.nick = name;
    cached_user.icon = icon;
    cached_user.desc = "";
    cached_user.sex = memochat::json::glaze_safe_get<int>(req, "sex", 0);
    gateauthsupport::CacheLoginProfile(email, cached_user);
    GateAsyncSideEffects::Instance().PublishUserProfileChanged(
        uid, memochat::json::glaze_safe_get<std::string>(result.data, "user_id", ""), email, name, name, icon, cached_user.sex);

    memolog::LogInfo("http2.user_register",
        "user registered via HTTP2 handler",
        {{"email", email}, {"uid", std::to_string(uid)}});
    return result;
}

LoginResult HandleResetPwd(const memochat::json::JsonValue& req) {
    LoginResult result;
    if (!memochat::json::glaze_has_key(req, "email") || !memochat::json::glaze_has_key(req, "user") ||
        !memochat::json::glaze_has_key(req, "passwd") || !memochat::json::glaze_has_key(req, "varifycode")) {
        result.error = 1;
        result.message = "missing required fields";
        return result;
    }

    const auto email = memochat::json::glaze_safe_get<std::string>(req, "email", "");
    const auto name = memochat::json::glaze_safe_get<std::string>(req, "user", "");
    const auto pwd = memochat::json::glaze_safe_get<std::string>(req, "passwd", "");
    const auto varify_code = memochat::json::glaze_safe_get<std::string>(req, "varifycode", "");

    std::string stored_code;
    if (!RedisMgr::GetInstance()->Get(CODEPREFIX + email, stored_code)) {
        result.error = 1002;
        result.message = "verification code expired";
        return result;
    }
    if (stored_code != varify_code) {
        result.error = 1003;
        result.message = "verification code mismatch";
        return result;
    }

    if (!PostgresMgr::GetInstance()->CheckEmail(name, email)) {
        result.error = ErrorCodes::EmailNotMatch;
        result.message = "user email mismatch";
        return result;
    }
    if (!PostgresMgr::GetInstance()->UpdatePwd(name, pwd)) {
        result.error = ErrorCodes::PasswdUpFailed;
        result.message = "password update failed";
        return result;
    }

    gateauthsupport::InvalidateLoginCacheByEmail(email);
    GateAsyncSideEffects::Instance().PublishCacheInvalidate(email, name, "reset_pwd");
    result.error = ErrorCodes::Success;
    result.data["email"] = email;
    result.data["user"] = name;
    memolog::LogInfo("http2.reset_pwd", "password reset via HTTP2 handler",
        {{"email", email}});
    return result;
}

LoginResult HandleUserLogin(const memochat::json::JsonValue& req) {
    LoginResult result;
    if (!memochat::json::glaze_has_key(req, "email") || !memochat::json::glaze_has_key(req, "passwd")) {
        result.error = 1;
        result.message = "missing email or password";
        return result;
    }

    const auto email = memochat::json::glaze_safe_get<std::string>(req, "email", "");
    const auto pwd = memochat::json::glaze_safe_get<std::string>(req, "passwd", "");
    const auto client_ver = memochat::json::glaze_safe_get<std::string>(req, "client_ver", "");

    result.data["min_version"] = gateauthsupport::MinClientVersion();
    result.data["feature_group_chat"] = true;

    if (!gateauthsupport::IsClientVersionAllowed(client_ver, gateauthsupport::MinClientVersion())) {
        result.error = ErrorCodes::ClientVersionTooLow;
        result.message = "client version too low";
        return result;
    }

    ::UserInfo dbUserInfo;
    gateauthsupport::UserInfo userInfo;
    bool login_cache_hit = gateauthsupport::TryLoadCachedLoginProfile(email, pwd, userInfo);
    bool pwd_valid = login_cache_hit;
    if (!pwd_valid) {
        pwd_valid = PostgresMgr::GetInstance()->CheckPwd(email, pwd, dbUserInfo);
        if (pwd_valid) {
            userInfo.uid = dbUserInfo.uid;
            userInfo.name = dbUserInfo.name;
            userInfo.email = dbUserInfo.email;
            userInfo.pwd = dbUserInfo.pwd;
            userInfo.nick = dbUserInfo.nick;
            userInfo.icon = dbUserInfo.icon;
            userInfo.desc = dbUserInfo.desc;
            userInfo.sex = dbUserInfo.sex;
            gateauthsupport::CacheLoginProfile(email, userInfo);
        }
    }
    if (!pwd_valid) {
        result.error = ErrorCodes::PasswdInvalid;
        result.message = "password invalid";
        return result;
    }

    auto route_nodes = gateauthsupport::LoadGateChatRouteNodes();
    if (route_nodes.empty()) {
        result.error = ErrorCodes::RPCFailed;
        result.message = "no chat server available";
        return result;
    }

    std::string http_token;
    const std::string token_key = USERTOKENPREFIX + std::to_string(userInfo.uid);
    if (!RedisMgr::GetInstance()->Get(token_key, http_token) || http_token.empty()) {
        http_token = boost::uuids::to_string(boost::uuids::random_generator()());
        RedisMgr::GetInstance()->Set(token_key, http_token);
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
    claims.expire_at_ms = claims.issued_at_ms +
        static_cast<int64_t>(gateauthsupport::GetChatTicketTtlSec()) * 1000;
    const std::string login_ticket = memochat::auth::EncodeTicket(
        claims, gateauthsupport::GetChatAuthSecret());

    if (login_ticket.empty()) {
        result.error = ErrorCodes::RPCFailed;
        result.message = "failed to issue login ticket";
        return result;
    }

    result.error = ErrorCodes::Success;
    result.data["protocol_version"] = gateauthsupport::LoginProtocolVersion();
    result.data["preferred_transport"] =
        (!route_nodes.front().quic_host.empty() && !route_nodes.front().quic_port.empty())
        ? "quic" : "tcp";
    result.data["fallback_transport"] = "tcp";
    result.data["email"] = email;
    result.data["uid"] = userInfo.uid;
    result.data["user_id"] = userInfo.user_id;
    result.data["token"] = http_token;
    result.data["host"] = route_nodes.front().host;
    result.data["port"] = route_nodes.front().port;
    result.data["login_ticket"] = login_ticket;
    result.data["ticket_expire_ms"] = static_cast<int64_t>(claims.expire_at_ms);
    result.data["user_profile"]["uid"] = userInfo.uid;
    result.data["user_profile"]["user_id"] = userInfo.user_id;
    result.data["user_profile"]["name"] = userInfo.name;
    result.data["user_profile"]["nick"] = userInfo.nick;
    result.data["user_profile"]["icon"] = userInfo.icon;
    result.data["user_profile"]["desc"] = userInfo.desc;
    result.data["user_profile"]["email"] = userInfo.email;
    result.data["user_profile"]["sex"] = userInfo.sex;
    for (const auto& route_node : route_nodes) {
        if (!route_node.quic_host.empty() && !route_node.quic_port.empty()) {
            memochat::json::JsonValue quic_ep;
            quic_ep["transport"] = "quic";
            quic_ep["host"] = route_node.quic_host;
            quic_ep["port"] = route_node.quic_port;
            quic_ep["server_name"] = route_node.name;
            quic_ep["priority"] = route_node.priority;
            memochat::json::glaze_append(result.data["chat_endpoints"], quic_ep);
        }
        memochat::json::JsonValue tcp_ep;
        tcp_ep["transport"] = "tcp";
        tcp_ep["host"] = route_node.host;
        tcp_ep["port"] = route_node.port;
        tcp_ep["server_name"] = route_node.name;
        tcp_ep["priority"] = route_node.priority;
        memochat::json::glaze_append(result.data["chat_endpoints"], tcp_ep);
    }

    GateAsyncSideEffects::Instance().PublishAuditLogin(userInfo.uid,
        userInfo.user_id, email, route_nodes.front().name,
        route_nodes.front().host, route_nodes.front().port, login_cache_hit);

    memolog::LogInfo("http2.user_login", "user login via HTTP2 handler",
        {{"uid", std::to_string(userInfo.uid)}, {"email", email},
         {"route", "/user_login"}, {"login_cache_hit", login_cache_hit ? "true" : "false"}});
    return result;
}

LoginResult HandleUserLogout(int uid, const std::string& token) {
    LoginResult result;
    if (uid <= 0 || token.empty()) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "invalid token";
        return result;
    }
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string stored_token;
    if (RedisMgr::GetInstance()->Get(token_key, stored_token) && stored_token == token) {
        RedisMgr::GetInstance()->Del(token_key);
    }
    result.error = ErrorCodes::Success;
    result.message = "logout success";
    memolog::LogInfo("http2.user_logout", "user logout via HTTP2 handler",
        {{"uid", std::to_string(uid)}});
    return result;
}

}  // namespace Http2AuthSupport

