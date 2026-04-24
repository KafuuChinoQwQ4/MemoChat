#include "GateHttpJsonSupport.h"

#include "AuthLoginSupport.h"
#include "CallService.h"
#include "ConfigMgr.h"
#include "GateAsyncSideEffects.h"
#include "GateHttpJsonSupport.h"
#include "GateRouteModules.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "VerifyGrpcClient.h"
#include "const.h"
#include "auth/ChatLoginTicket.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdlib>
#include <functional>
#include "json/GlazeCompat.h"

namespace beast = boost::beast;
namespace http = beast::http;

namespace {

gateauthsupport::UserInfo ToGateAuthUserInfo(const UserInfo& src) {
    gateauthsupport::UserInfo dst;
    dst.name = src.name;
    dst.pwd = src.pwd;
    dst.uid = src.uid;
    dst.user_id = src.user_id;
    dst.email = src.email;
    dst.nick = src.nick;
    dst.icon = src.icon;
    dst.desc = src.desc;
    dst.sex = src.sex;
    return dst;
}

}  // namespace

void AuthHttpService::RegisterRoutes(LogicSystem& logic) {
    logic.RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        memochat::json::JsonValue root;
        memochat::json::JsonValue src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            memolog::LogWarn("gate.get_varifycode.invalid_json", "request json parse failed");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (!isMember(src_root, "email")) {
            memolog::LogWarn("gate.get_varifycode.invalid_body", "email is missing");
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const auto email = src_root["email"].asString();
        GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        root["error"] = rsp.error();
        root["email"] = src_root["email"];
        memolog::LogInfo("gate.get_varifycode", "verify code requested",
            {{"route", "/get_varifycode"}, {"email", email}, {"error_code", std::to_string(rsp.error())}});
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        memochat::json::JsonValue root;
        memochat::json::JsonValue src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            memolog::LogWarn("gate.user_register.invalid_json", "request json parse failed");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const auto email = src_root["email"].asString();
        const auto name = src_root["user"].asString();
        const auto pwd = src_root["passwd"].asString();
        const auto confirm = src_root["confirm"].asString();
        const auto icon = src_root["icon"].asString();

        if (pwd != confirm) {
            memolog::LogWarn("gate.user_register.failed", "password mismatch", {{"email", email}});
            root["error"] = ErrorCodes::PasswdErr;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::string varify_code;
        if (!RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code)) {
            memolog::LogWarn("gate.user_register.failed", "verify code expired", {{"email", email}});
            root["error"] = ErrorCodes::VarifyExpired;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (varify_code != src_root["varifycode"].asString()) {
            memolog::LogWarn("gate.user_register.failed", "verify code mismatch", {{"email", email}});
            root["error"] = ErrorCodes::VarifyCodeErr;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const int uid = PostgresMgr::GetInstance()->RegUser(name, email, pwd, icon);
        if (uid == 0 || uid == -1) {
            memolog::LogWarn("gate.user_register.failed", "user or email exists", {{"email", email}, {"name", name}});
            root["error"] = ErrorCodes::UserExist;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        root["error"] = 0;
        root["uid"] = uid;
        root["user_id"] = PostgresMgr::GetInstance()->GetUserPublicId(uid);
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
        GateAsyncSideEffects::Instance().PublishUserProfileChanged(uid,
            root["user_id"].asString(),
            email,
            name,
            name,
            icon,
            cached_user.sex);
        memolog::LogInfo("gate.user_register", "user registered", {{"email", email}, {"uid", std::to_string(uid)}});
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        memochat::json::JsonValue root;
        memochat::json::JsonValue src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            memolog::LogWarn("gate.reset_pwd.invalid_json", "request json parse failed");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const auto email = src_root["email"].asString();
        const auto name = src_root["user"].asString();
        const auto pwd = src_root["passwd"].asString();

        std::string varify_code;
        if (!RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code)) {
            memolog::LogWarn("gate.reset_pwd.failed", "verify code expired", {{"email", email}});
            root["error"] = ErrorCodes::VarifyExpired;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (varify_code != src_root["varifycode"].asString()) {
            memolog::LogWarn("gate.reset_pwd.failed", "verify code mismatch", {{"email", email}});
            root["error"] = ErrorCodes::VarifyCodeErr;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (!PostgresMgr::GetInstance()->CheckEmail(name, email)) {
            memolog::LogWarn("gate.reset_pwd.failed", "user email mismatch", {{"email", email}, {"name", name}});
            root["error"] = ErrorCodes::EmailNotMatch;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (!PostgresMgr::GetInstance()->UpdatePwd(name, pwd)) {
            memolog::LogWarn("gate.reset_pwd.failed", "password update failed", {{"email", email}});
            root["error"] = ErrorCodes::PasswdUpFailed;
            beast::ostream(connection->_response.body()) << root.toStyledString();
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
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        const auto login_start_ms = gateauthsupport::NowMs();
        memochat::json::JsonValue root;
        memochat::json::JsonValue src_root;
        root["trace_id"] = connection->_trace_id;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            memolog::LogWarn("gate.user_login.invalid_json", "request json parse failed");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const auto email = src_root["email"].asString();
        const auto pwd = src_root["passwd"].asString();
        const auto client_ver = src_root.get("client_ver", "").asString();
        root["min_version"] = gateauthsupport::MinClientVersion();
        root["feature_group_chat"] = true;
        memolog::LogInfo("gate.user_login.version_check", "version check",
            {{"client_ver", client_ver}, {"min_ver", gateauthsupport::MinClientVersion()}});
        if (!gateauthsupport::IsClientVersionAllowed(client_ver, gateauthsupport::MinClientVersion())) {
            root["error"] = ErrorCodes::ClientVersionTooLow;
            memolog::LogWarn("gate.user_login.failed", "client version too low",
                {{"email", email}, {"error_code", std::to_string(ErrorCodes::ClientVersionTooLow)}});
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        gateauthsupport::UserInfo userInfo;
        const auto mysql_start_ms = gateauthsupport::NowMs();
        gateauthsupport::UserInfo tempUser;
        bool login_cache_hit = gateauthsupport::TryLoadCachedLoginProfile(email, pwd, tempUser);
        if (login_cache_hit) {
            userInfo = tempUser;
        }
        bool pwd_valid = login_cache_hit;
        int64_t mysql_check_pwd_ms = 0;
        if (!pwd_valid) {
            UserInfo dbUser;
            pwd_valid = PostgresMgr::GetInstance()->CheckPwd(email, pwd, dbUser);
            mysql_check_pwd_ms = gateauthsupport::NowMs() - mysql_start_ms;
            if (pwd_valid) {
                userInfo.name = dbUser.name;
                userInfo.pwd = dbUser.pwd;
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
        if (!pwd_valid) {
            memolog::LogWarn("gate.user_login.failed", "password invalid",
                {{"email", email}, {"error_code", std::to_string(ErrorCodes::PasswdInvalid)}});
            root["error"] = ErrorCodes::PasswdInvalid;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::vector<std::string> server_load_snapshot;
        std::vector<std::string> least_loaded_servers;
        const auto route_start_ms = gateauthsupport::NowMs();
        const auto route_nodes = gateauthsupport::LoadGateChatRouteNodes(&server_load_snapshot, &least_loaded_servers);
        const auto route_select_ms = gateauthsupport::NowMs() - route_start_ms;
        if (route_nodes.empty()) {
            memolog::LogWarn("gate.user_login.failed", "no chat server available",
                {{"uid", std::to_string(userInfo.uid)}, {"error_code", std::to_string(ErrorCodes::RPCFailed)}});
            root["error"] = ErrorCodes::RPCFailed;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const auto ticket_start_ms = gateauthsupport::NowMs();
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
        claims.expire_at_ms = claims.issued_at_ms + static_cast<int64_t>(gateauthsupport::GetChatTicketTtlSec()) * 1000;
        const std::string login_ticket = memochat::auth::EncodeTicket(claims, gateauthsupport::GetChatAuthSecret());
        const auto ticket_issue_ms = gateauthsupport::NowMs() - ticket_start_ms;
        if (login_ticket.empty()) {
            root["error"] = ErrorCodes::RPCFailed;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        memolog::TraceContext::SetUid(std::to_string(userInfo.uid));
        root["error"] = 0;
        root["protocol_version"] = gateauthsupport::LoginProtocolVersion();
        root["preferred_transport"] = (!route_nodes.front().quic_host.empty() && !route_nodes.front().quic_port.empty()) ? "quic" : "tcp";
        root["fallback_transport"] = "tcp";
        root["email"] = email;
        root["uid"] = userInfo.uid;
        root["user_id"] = userInfo.user_id;
        root["token"] = http_token;
        root["host"] = route_nodes.front().host;
        root["port"] = route_nodes.front().port;
        root["login_ticket"] = login_ticket;
        root["ticket_expire_ms"] = static_cast<int64_t>(claims.expire_at_ms);
        root["user_profile"]["uid"] = userInfo.uid;
        root["user_profile"]["user_id"] = userInfo.user_id;
        root["user_profile"]["name"] = userInfo.name;
        root["user_profile"]["nick"] = userInfo.nick;
        root["user_profile"]["icon"] = userInfo.icon;
        root["user_profile"]["desc"] = userInfo.desc;
        root["user_profile"]["email"] = userInfo.email;
        root["user_profile"]["sex"] = userInfo.sex;
        auto chat_endpoints_arr = root["chat_endpoints"];
        for (const auto& route_node : route_nodes) {
            if (!route_node.quic_host.empty() && !route_node.quic_port.empty()) {
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
        root["stage_metrics"]["mysql_check_pwd_ms"] = static_cast<int64_t>(mysql_check_pwd_ms);
        root["stage_metrics"]["route_select_ms"] = static_cast<int64_t>(route_select_ms);
        root["stage_metrics"]["ticket_issue_ms"] = static_cast<int64_t>(ticket_issue_ms);
        root["stage_metrics"]["user_login_total_ms"] = static_cast<int64_t>(gateauthsupport::NowMs() - login_start_ms);
        memolog::LogInfo("gate.user_login", "user login succeeded",
            {
                {"uid", std::to_string(userInfo.uid)},
                {"route", "/user_login"},
                {"chat_host", route_nodes.front().host},
                {"chat_port", route_nodes.front().port},
                {"chat_server", route_nodes.front().name},
                {"login_cache_hit", login_cache_hit ? "true" : "false"},
                {"mysql_check_pwd_ms", std::to_string(mysql_check_pwd_ms)},
                {"route_select_ms", std::to_string(route_select_ms)},
                {"ticket_issue_ms", std::to_string(ticket_issue_ms)},
                {"user_login_total_ms", std::to_string(gateauthsupport::NowMs() - login_start_ms)},
                {"server_loads", [&server_load_snapshot]() {
                    std::ostringstream oss;
                    for (size_t i = 0; i < server_load_snapshot.size(); ++i) {
                        if (i > 0) {
                            oss << ",";
                        }
                        oss << server_load_snapshot[i];
                    }
                    return oss.str();
                }()}
            });
        memolog::LogInfo("login.stage.summary", "gate login stage summary",
            {
                {"uid", std::to_string(userInfo.uid)},
                {"login_cache_hit", login_cache_hit ? "true" : "false"},
                {"mysql_check_pwd_ms", std::to_string(mysql_check_pwd_ms)},
                {"route_select_ms", std::to_string(route_select_ms)},
                {"ticket_issue_ms", std::to_string(ticket_issue_ms)},
                {"user_login_total_ms", std::to_string(gateauthsupport::NowMs() - login_start_ms)}
            });
        GateAsyncSideEffects::Instance().PublishAuditLogin(userInfo.uid,
            userInfo.user_id,
            email,
            route_nodes.front().name,
            route_nodes.front().host,
            route_nodes.front().port,
            login_cache_hit);
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });
}

void ProfileHttpService::RegisterRoutes(LogicSystem& logic)
{
    logic.RegPost("/user_update_profile", [](std::shared_ptr<HttpConnection> connection) {
        memochat::json::JsonValue root;
        memochat::json::JsonValue src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (!isMember(src_root, "uid") || !isMember(src_root, "nick")
            || !isMember(src_root, "desc") || !isMember(src_root, "icon")) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const auto uid = src_root["uid"].asInt();
        const auto name = src_root.get("name", "").asString();
        const auto nick = src_root["nick"].asString();
        const auto desc = src_root["desc"].asString();
        const auto icon = src_root["icon"].asString();
        if (uid <= 0 || nick.empty()) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (!PostgresMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon)) {
            root["error"] = ErrorCodes::ProfileUpFailed;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        RedisMgr::GetInstance()->Del("ubaseinfo_" + std::to_string(uid));
        if (!name.empty()) {
            RedisMgr::GetInstance()->Del("nameinfo_" + name);
        }
        gateauthsupport::InvalidateLoginCacheByUid(uid);

        root["error"] = ErrorCodes::Success;
        root["uid"] = uid;
        root["name"] = name;
        root["nick"] = nick;
        root["desc"] = desc;
        root["icon"] = icon;
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });
}

void CallHttpServiceRoutes::RegisterRoutes(LogicSystem& logic)
{
    logic.RegPost("/api/call/start", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection, [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            memolog::LogInfo("call.start.requested", "call start requested", {{"trace_id", trace_id}, {"module", "call"}});
            return CallService::GetInstance()->StartCall(src_root, root, trace_id);
        });
    });
    logic.RegPost("/api/call/accept", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection, [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->AcceptCall(src_root, root, trace_id);
        });
    });
    logic.RegPost("/api/call/reject", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection, [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->RejectCall(src_root, root, trace_id);
        });
    });
    logic.RegPost("/api/call/cancel", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection, [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->CancelCall(src_root, root, trace_id);
        });
    });
    logic.RegPost("/api/call/hangup", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection, [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->HangupCall(src_root, root, trace_id);
        });
    });
    logic.RegGet("/api/call/token", [](std::shared_ptr<HttpConnection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        const int uid = std::atoi(connection->_get_params["uid"].c_str());
        const std::string token = connection->_get_params["token"];
        const std::string call_id = connection->_get_params["call_id"];
        const std::string role = connection->_get_params["role"];
        memochat::json::JsonValue root;
        CallService::GetInstance()->GetToken(uid, token, call_id, role, root, connection->_trace_id);
        std::string json_str = memochat::json::glaze_stringify(root);
        if (json_str.find("\"trace_id\"") == std::string::npos) {
            if (json_str.back() == '}') {
                json_str.pop_back();
                json_str += ",\"trace_id\":\"" + connection->_trace_id + "\"}";
            }
        }
        beast::ostream(connection->_response.body()) << json_str;
        return true;
    });
    logic.RegPost("/api/call/token", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection, [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            const int uid = memochat::json::glaze_safe_get<int>(src_root, "uid", 0);
            const std::string token = memochat::json::glaze_safe_get<std::string>(src_root, "token", "");
            const std::string call_id = memochat::json::glaze_safe_get<std::string>(src_root, "call_id", "");
            const std::string role = memochat::json::glaze_safe_get<std::string>(src_root, "role", "");
            return CallService::GetInstance()->GetToken(uid, token, call_id, role, root, trace_id);
        });
    });
}
