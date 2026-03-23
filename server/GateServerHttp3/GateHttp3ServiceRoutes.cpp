#include "GateHttp3ServiceRoutes.h"

#include "GateHttp3JsonSupport.h"
#include "GateHttp3Connection.h"
#include "CallService.h"
#include "LogicSystem.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "VerifyGrpcClient.h"
#include "GateAsyncSideEffects.h"
#include "AuthLoginSupport.h"
#include "../GateServerDrogon/DrogonMediaSupport.h"
#include "../GateServerDrogon/DrogonProfileSupport.h"

#include "const.h"
#include "auth/ChatLoginTicket.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <json/json.h>

namespace GateHttp3ServiceImpl {

void RegisterRoutes(LogicSystem& logic) {
    logic.RegPost("/get_varifycode", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            (void)trace_id;
            if (!src_root.isMember("email")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            const auto email = src_root["email"].asString();
            GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
            root["error"] = rsp.error();
            root["email"] = src_root["email"];
            memolog::LogInfo("gate.http3.get_varifycode", "HTTP/3 verify code requested",
                {{"email", email}, {"error_code", std::to_string(rsp.error())}});
            return true;
        });
    });

    logic.RegPost("/user_register", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            (void)trace_id;
            const auto email = src_root["email"].asString();
            const auto name = src_root["user"].asString();
            const auto pwd = src_root["passwd"].asString();
            const auto confirm = src_root["confirm"].asString();
            const auto icon = src_root["icon"].asString();

            if (pwd != confirm) {
                root["error"] = ErrorCodes::PasswdErr;
                return false;
            }

            std::string varify_code;
            if (!RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code)) {
                root["error"] = ErrorCodes::VarifyExpired;
                return false;
            }
            if (varify_code != src_root["varifycode"].asString()) {
                root["error"] = ErrorCodes::VarifyCodeErr;
                return false;
            }

            const int uid = PostgresMgr::GetInstance()->RegUser(name, email, pwd, icon);
            if (uid == 0 || uid == -1) {
                root["error"] = ErrorCodes::UserExist;
                return false;
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
            memolog::LogInfo("gate.http3.user_register", "HTTP/3 user registered",
                {{"email", email}, {"uid", std::to_string(uid)}});
            return true;
        });
    });

    logic.RegPost("/reset_pwd", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            (void)trace_id;
            if (!src_root.isMember("email")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            const auto email = src_root["email"].asString();
            GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
            root["error"] = rsp.error();
            root["email"] = email;
            return true;
        });
    });

    logic.RegPost("/user_login", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            memolog::TraceContext::SetTraceId(trace_id);
            const auto email = src_root["email"].asString();
            const auto pwd = src_root["passwd"].asString();
            const auto client_ver = src_root.get("client_ver", "").asString();
            root["min_version"] = gateauthsupport::MinClientVersion();
            root["feature_group_chat"] = true;
            if (!gateauthsupport::IsClientVersionAllowed(client_ver, gateauthsupport::MinClientVersion())) {
                root["error"] = ErrorCodes::ClientVersionTooLow;
                memolog::LogWarn("gate.http3.user_login.version_low", "client version too low",
                    {{"email", email}, {"error_code", std::to_string(ErrorCodes::ClientVersionTooLow)}});
                return false;
            }
            gateauthsupport::UserInfo userInfo;
            gateauthsupport::UserInfo tempUser;
            bool login_cache_hit = gateauthsupport::TryLoadCachedLoginProfile(email, pwd, tempUser);
            if (login_cache_hit) {
                userInfo = tempUser;
            }
            bool pwd_valid = login_cache_hit;
            if (!pwd_valid) {
                UserInfo dbUser;
                pwd_valid = PostgresMgr::GetInstance()->CheckPwd(email, pwd, dbUser);
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
                memolog::LogWarn("gate.http3.user_login.pwd_invalid", "password invalid",
                    {{"email", email}, {"error_code", std::to_string(ErrorCodes::PasswdInvalid)}});
                root["error"] = ErrorCodes::PasswdInvalid;
                return false;
            }
            auto route_nodes = gateauthsupport::LoadGateChatRouteNodes();
            if (route_nodes.empty()) {
                memolog::LogWarn("gate.http3.user_login.no_server", "no chat server available",
                    {{"uid", std::to_string(userInfo.uid)}});
                root["error"] = ErrorCodes::RPCFailed;
                return false;
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
            claims.expire_at_ms = claims.issued_at_ms + static_cast<int64_t>(gateauthsupport::GetChatTicketTtlSec()) * 1000;
            const std::string login_ticket = memochat::auth::EncodeTicket(claims, gateauthsupport::GetChatAuthSecret());
            if (login_ticket.empty()) {
                root["error"] = ErrorCodes::RPCFailed;
                return false;
            }
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
            root["ticket_expire_ms"] = static_cast<Json::Int64>(claims.expire_at_ms);
            root["user_profile"]["uid"] = userInfo.uid;
            root["user_profile"]["user_id"] = userInfo.user_id;
            root["user_profile"]["name"] = userInfo.name;
            root["user_profile"]["nick"] = userInfo.nick;
            root["user_profile"]["icon"] = userInfo.icon;
            root["user_profile"]["desc"] = userInfo.desc;
            root["user_profile"]["email"] = userInfo.email;
            root["user_profile"]["sex"] = userInfo.sex;
            for (const auto& route_node : route_nodes) {
                if (!route_node.quic_host.empty() && !route_node.quic_port.empty()) {
                    Json::Value quic_ep;
                    quic_ep["transport"] = "quic";
                    quic_ep["host"] = route_node.quic_host;
                    quic_ep["port"] = route_node.quic_port;
                    quic_ep["server_name"] = route_node.name;
                    quic_ep["priority"] = route_node.priority;
                    root["chat_endpoints"].append(quic_ep);
                }
                Json::Value tcp_ep;
                tcp_ep["transport"] = "tcp";
                tcp_ep["host"] = route_node.host;
                tcp_ep["port"] = route_node.port;
                tcp_ep["server_name"] = route_node.name;
                tcp_ep["priority"] = route_node.priority;
                root["chat_endpoints"].append(tcp_ep);
            }
            memolog::LogInfo("gate.http3.user_login", "HTTP/3 user login succeeded",
                {{"uid", std::to_string(userInfo.uid)}, {"email", email},
                 {"route", "/user_login"}, {"trace_id", trace_id}});
            GateAsyncSideEffects::Instance().PublishAuditLogin(userInfo.uid,
                userInfo.user_id, email, route_nodes.front().name,
                route_nodes.front().host, route_nodes.front().port, login_cache_hit);
            return true;
        });
    });

    logic.RegPost("/user_update_profile", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            (void)trace_id;
            if (!src_root.isMember("uid") || !src_root.isMember("nick")
                || !src_root.isMember("desc") || !src_root.isMember("icon")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            const auto uid = src_root["uid"].asInt();
            const auto name = src_root.get("name", "").asString();
            const auto nick = src_root["nick"].asString();
            const auto desc = src_root["desc"].asString();
            const auto icon = src_root["icon"].asString();
            if (uid <= 0 || nick.empty()) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            if (!PostgresMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon)) {
                root["error"] = ErrorCodes::ProfileUpFailed;
                return false;
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
            return true;
        });
    });

    logic.RegGet("/healthz", [](std::shared_ptr<GateHttp3Connection> connection) {
        Json::Value root;
        root["status"] = "ok";
        root["service"] = "GateServer-HTTP3";
        connection->SendResponse(200, root.toStyledString(), "application/json");
        return true;
    });

    logic.RegGet("/readyz", [](std::shared_ptr<GateHttp3Connection> connection) {
        Json::Value root;
        root["status"] = "ready";
        root["service"] = "GateServer-HTTP3";
        connection->SendResponse(200, root.toStyledString(), "application/json");
        return true;
    });

    logic.RegPost("/api/call/token", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string call_id = src_root.get("call_id", "").asString();
            const std::string role = src_root.get("role", "").asString();
            return CallService::GetInstance()->GetToken(uid, token, call_id, role, root, trace_id);
        });
    });

    logic.RegPost("/api/call/start", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->StartCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/accept", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->AcceptCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/reject", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->RejectCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/cancel", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->CancelCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/hangup", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
            return CallService::GetInstance()->HangupCall(src_root, root, trace_id);
        });
    });

    // Media upload routes
    logic.RegPost("/upload_media_init", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string media_type = src_root.get("media_type", "file").asString();
            const std::string file_name = src_root.get("file_name", "").asString();
            const std::string mime = src_root.get("mime", "").asString();
            const int64_t file_size = src_root.get("file_size", 0).asInt64();
            auto result = DrogonMediaSupport::HandleUploadMediaInit(uid, token, media_type, file_name, mime, file_size);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegPost("/upload_media_chunk", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string upload_id = src_root.get("upload_id", "").asString();
            const int index = src_root.get("index", -1).asInt();
            const std::string chunk_data_base64 = src_root.get("data_base64", "").asString();
            auto result = DrogonMediaSupport::HandleUploadMediaChunk(uid, token, upload_id, index, chunk_data_base64);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegPost("/upload_media_complete", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string upload_id = src_root.get("upload_id", "").asString();
            auto result = DrogonMediaSupport::HandleUploadMediaComplete(uid, token, upload_id);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegPost("/upload_media", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string media_type = src_root.get("media_type", "file").asString();
            const std::string file_name = src_root.get("file_name", "").asString();
            const std::string mime = src_root.get("mime", "").asString();
            const std::string data_base64 = src_root.get("data_base64", "").asString();
            auto result = DrogonMediaSupport::HandleUploadMediaSimple(uid, token, media_type, file_name, mime, data_base64);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegGet("/upload_media_status", [](std::shared_ptr<GateHttp3Connection> connection) {
        int uid = 0;
        std::string token;
        std::string upload_id;
        std::string uid_str = connection->GetQueryParam("uid");
        if (!uid_str.empty()) uid = std::atoi(uid_str.c_str());
        token = connection->GetQueryParam("token");
        upload_id = connection->GetQueryParam("upload_id");
        if (uid == 0 || token.empty() || upload_id.empty()) {
            const std::string body = connection->GetRequestBody();
            if (!body.empty()) {
                Json::Value src_root;
                Json::Reader reader;
                if (reader.parse(body, src_root)) {
                    if (uid == 0) uid = src_root.get("uid", 0).asInt();
                    if (token.empty()) token = src_root.get("token", "").asString();
                    if (upload_id.empty()) upload_id = src_root.get("upload_id", "").asString();
                }
            }
        }
        auto result = DrogonMediaSupport::HandleUploadMediaStatus(uid, token, upload_id);
        Json::Value root = result.data;
        root["error"] = result.error;
        if (!result.message.empty()) root["message"] = result.message;
        connection->SendResponse(result.error == 0 ? 200 : 400, root.toStyledString(), "application/json");
        return true;
    });

    logic.RegGet("/media/download", [](std::shared_ptr<GateHttp3Connection> connection) {
        int uid = 0;
        std::string token;
        std::string media_key;
        std::string uid_str = connection->GetQueryParam("uid");
        if (!uid_str.empty()) uid = std::atoi(uid_str.c_str());
        token = connection->GetQueryParam("token");
        media_key = connection->GetQueryParam("asset");
        if (uid <= 0 || token.empty() || media_key.empty()) {
            Json::Value root;
            root["error"] = 1;
            root["message"] = "missing media key or auth params";
            connection->SendResponse(400, root.toStyledString(), "application/json");
            return true;
        }
        auto result = DrogonMediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
        if (result.error != 0) {
            Json::Value root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            connection->SendResponse(400, root.toStyledString(), "application/json");
            return true;
        }
        if (result.data.isMember("redirect")) {
            Json::Value root;
            root["error"] = 0;
            root["redirect"] = result.data["redirect"].asString();
            root["content_type"] = result.data["content_type"].asString();
            root["file_name"] = result.data["file_name"].asString();
            connection->SendResponse(200, root.toStyledString(), "application/json");
            return true;
        }
        if (result.data.isMember("path")) {
            Json::Value root;
            root["error"] = 0;
            root["path"] = result.data["path"].asString();
            root["content_type"] = result.data["content_type"].asString();
            root["file_name"] = result.data["file_name"].asString();
            root["size"] = result.data["size"];
            connection->SendResponse(200, root.toStyledString(), "application/json");
            return true;
        }
        Json::Value root;
        root["error"] = 1;
        root["message"] = "file not found";
        connection->SendResponse(404, root.toStyledString(), "application/json");
        return true;
    });

    // User profile routes
    logic.RegPost("/get_user_info", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            if (uid <= 0) {
                root["error"] = 1;
                root["message"] = "invalid uid";
                return false;
            }
            auto result = DrogonProfileSupport::HandleGetUserInfo(uid);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });
}

}  // namespace GateHttp3ServiceImpl

void GateHttp3Service::RegisterRoutes(LogicSystem& logic) {
    GateHttp3ServiceImpl::RegisterRoutes(logic);
}
