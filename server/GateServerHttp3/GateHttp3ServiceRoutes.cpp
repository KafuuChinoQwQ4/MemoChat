#include "GateHttp3ServiceRoutes.h"

#include "GateHttp3JsonSupport.h"
#include "GateHttp3Connection.h"
#include "CallService.h"
#include "LogicSystem.h"
#include "PostgresMgr.h"
#include "MongoMgr.h"
#include "RedisMgr.h"
#include "VerifyGrpcClient.h"
#include "GateAsyncSideEffects.h"
#include "AuthLoginSupport.h"
#include "Http2MediaSupport.h"
#include "Http2ProfileSupport.h"

#include "const.h"
#include "auth/ChatLoginTicket.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include "json/GlazeCompat.h"
#include <chrono>

namespace GateHttp3ServiceImpl {

namespace {

void NormalizeMomentItem(MomentItemInfo& item) {
    if (item.media_type != "image" && item.media_type != "video") {
        item.media_type = "text";
    }
    if ((item.media_type == "image" || item.media_type == "video") && item.media_key.empty()) {
        item.media_type = "text";
    }
    if (item.media_type == "text") {
        item.media_key.clear();
        item.thumb_key.clear();
        item.width = 0;
        item.height = 0;
        item.duration_ms = 0;
    }
}

void FillCommentLikeJson(int viewer_uid, int64_t comment_id, memochat::json::JsonValue& json) {
    std::vector<MomentLikeInfo> likes;
    bool has_more = false;
    PostgresMgr::GetInstance()->GetMomentCommentLikes(comment_id, 100, likes, has_more);
    json["has_liked"] = PostgresMgr::GetInstance()->HasLikedMomentComment(comment_id, viewer_uid);
    json["like_count"] = static_cast<int>(likes.size());

    memochat::json::JsonValue like_names(memochat::json::array_t{});
    memochat::json::JsonValue likes_arr(memochat::json::array_t{});
    for (const auto& like : likes) {
        if (!like.user_nick.empty()) {
            like_names.append(like.user_nick);
        }
        memochat::json::JsonValue like_json;
        like_json["uid"] = like.uid;
        like_json["user_nick"] = like.user_nick;
        like_json["user_icon"] = like.user_icon;
        like_json["created_at"] = static_cast<int64_t>(like.created_at);
        likes_arr.append(like_json);
    }
    json["like_names"] = like_names;
    json["likes"] = likes_arr;
}

} // namespace

void RegisterRoutes(LogicSystem& logic) {
    logic.RegPost("/get_varifycode", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
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
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
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
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
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
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
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
            root["ticket_expire_ms"] = static_cast<int64_t>(claims.expire_at_ms);
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
                    memochat::json::JsonValue quic_ep;
                    quic_ep["transport"] = "quic";
                    quic_ep["host"] = route_node.quic_host;
                    quic_ep["port"] = route_node.quic_port;
                    quic_ep["server_name"] = route_node.name;
                    quic_ep["priority"] = route_node.priority;
                    root["chat_endpoints"].append(quic_ep);
                }
                memochat::json::JsonValue tcp_ep;
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
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
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
        memochat::json::JsonValue root;
        root["status"] = "ok";
        root["service"] = "GateServer-HTTP3";
        connection->SendResponse(200, root.toStyledString(), "application/json");
        return true;
    });

    logic.RegGet("/readyz", [](std::shared_ptr<GateHttp3Connection> connection) {
        memochat::json::JsonValue root;
        root["status"] = "ready";
        root["service"] = "GateServer-HTTP3";
        connection->SendResponse(200, root.toStyledString(), "application/json");
        return true;
    });

    logic.RegPost("/api/call/token", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string call_id = src_root.get("call_id", "").asString();
            const std::string role = src_root.get("role", "").asString();
            return CallService::GetInstance()->GetToken(uid, token, call_id, role, root, trace_id);
        });
    });

    logic.RegPost("/api/call/start", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->StartCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/accept", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->AcceptCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/reject", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->RejectCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/cancel", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->CancelCall(src_root, root, trace_id);
        });
    });

    logic.RegPost("/api/call/hangup", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
            return CallService::GetInstance()->HangupCall(src_root, root, trace_id);
        });
    });

    // Media upload routes
    logic.RegPost("/upload_media_init", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string media_type = src_root.get("media_type", "file").asString();
            const std::string file_name = src_root.get("file_name", "").asString();
            const std::string mime = src_root.get("mime", "").asString();
            const int64_t file_size = src_root.get("file_size", 0).asInt64();
            auto result = Http2MediaSupport::HandleUploadMediaInit(uid, token, media_type, file_name, mime, file_size);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegPost("/upload_media_chunk", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string upload_id = src_root.get("upload_id", "").asString();
            const int index = src_root.get("index", -1).asInt();
            const std::string chunk_data_base64 = src_root.get("data_base64", "").asString();
            auto result = Http2MediaSupport::HandleUploadMediaChunk(uid, token, upload_id, index, chunk_data_base64);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegPost("/upload_media_complete", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string upload_id = src_root.get("upload_id", "").asString();
            auto result = Http2MediaSupport::HandleUploadMediaComplete(uid, token, upload_id);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    logic.RegPost("/upload_media", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            const std::string token = src_root.get("token", "").asString();
            const std::string media_type = src_root.get("media_type", "file").asString();
            const std::string file_name = src_root.get("file_name", "").asString();
            const std::string mime = src_root.get("mime", "").asString();
            const std::string data_base64 = src_root.get("data_base64", "").asString();
            auto result = Http2MediaSupport::HandleUploadMediaSimple(uid, token, media_type, file_name, mime, data_base64);
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
                memochat::json::JsonValue src_root;
                if (memochat::json::glaze_parse(src_root, body)) {
                    if (uid == 0) uid = src_root.get("uid", 0).asInt();
                    if (token.empty()) token = src_root.get("token", "").asString();
                    if (upload_id.empty()) upload_id = src_root.get("upload_id", "").asString();
                }
            }
        }
        auto result = Http2MediaSupport::HandleUploadMediaStatus(uid, token, upload_id);
        memochat::json::JsonValue root = result.data;
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
            memochat::json::JsonValue root;
            root["error"] = 1;
            root["message"] = "missing media key or auth params";
            connection->SendResponse(400, root.toStyledString(), "application/json");
            return true;
        }
        auto result = Http2MediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
        if (result.error != 0) {
            memochat::json::JsonValue root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            connection->SendResponse(400, root.toStyledString(), "application/json");
            return true;
        }
        if (result.data.isMember("data")) {
            const std::string& ct = result.data["content_type"].asString();
            const std::string& body_data = result.data["data"].asString();
            connection->SendResponse(200, body_data, ct);
            return true;
        }
        if (result.data.isMember("path")) {
            memochat::json::JsonValue root;
            root["error"] = 0;
            root["path"] = result.data["path"].asString();
            root["content_type"] = result.data["content_type"].asString();
            root["file_name"] = result.data["file_name"].asString();
            root["size"] = result.data["size"];
            connection->SendResponse(200, root.toStyledString(), "application/json");
            return true;
        }
        memochat::json::JsonValue root;
        root["error"] = 1;
        root["message"] = result.message.empty() ? "file not found" : result.message;
        connection->SendResponse(404, root.toStyledString(), "application/json");
        return true;
    });

    // User profile routes
    logic.RegPost("/get_user_info", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            const int uid = src_root.get("uid", 0).asInt();
            if (uid <= 0) {
                root["error"] = 1;
                root["message"] = "invalid uid";
                return false;
            }
            auto result = Http2ProfileSupport::HandleGetUserInfo(uid);
            root = result.data;
            root["error"] = result.error;
            if (!result.message.empty()) root["message"] = result.message;
            return result.error == 0;
        });
    });

    // Moments routes (HTTP/3)
    logic.RegPost("/api/moments/publish", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            if (uid <= 0) {
                root["error"] = ErrorCodes::UidInvalid;
                return false;
            }

            int visibility = src_root.get("visibility", 0).asInt();
            std::string location = src_root.get("location", "").asString();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            MomentInfo moment;
            moment.uid = uid;
            moment.visibility = visibility;
            moment.location = location;
            moment.created_at = now_ms;

            int64_t moment_id = 0;
            if (!PostgresMgr::GetInstance()->AddMoment(moment, &moment_id)) {
                root["error"] = ErrorCodes::RPCFailed;
                return false;
            }

            MomentContentInfo content;
            content.moment_id = moment_id;
            content.server_time = now_ms;
            if (src_root.isMember("items") && src_root["items"].isArray()) {
                for (const auto& item_json : src_root["items"]) {
                    MomentItemInfo item;
                    item.media_type = item_json.get("media_type", "text").asString();
                    item.media_key = item_json.get("media_key", "").asString();
                    item.thumb_key = item_json.get("thumb_key", "").asString();
                    item.content = item_json.get("content", "").asString();
                    item.width = item_json.get("width", 0).asInt();
                    item.height = item_json.get("height", 0).asInt();
                    item.duration_ms = item_json.get("duration_ms", 0).asInt();
                    NormalizeMomentItem(item);
                    content.items.push_back(item);
                }
            }
            const std::string raw_content = src_root.get("content", "").asString();
            if (content.items.empty() && !raw_content.empty()) {
                MomentItemInfo item;
                item.media_type = "text";
                item.content = raw_content;
                NormalizeMomentItem(item);
                content.items.push_back(item);
            }
            if (moment_id > 0 && !MongoMgr::GetInstance()->InsertMomentContent(content)) {
                PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid);
                root["error"] = ErrorCodes::RPCFailed;
                memolog::LogError("gate.http3.moments.publish", "failed to store moment content in MongoDB",
                    { {"moment_id", std::to_string(moment_id)}, {"uid", std::to_string(uid)} });
                return false;
            }

            root["error"] = ErrorCodes::Success;
            root["moment_id"] = static_cast<int64_t>(moment_id);
            return true;
        });
    });

    logic.RegPost("/api/moments/list", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            if (uid <= 0) {
                root["error"] = ErrorCodes::UidInvalid;
                return false;
            }

            int64_t last_moment_id = src_root.get("last_moment_id", 0).asInt64();
            int limit = src_root.get("limit", 20).asInt();
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;

            std::vector<MomentInfo> moments;
            bool has_more = false;
            PostgresMgr::GetInstance()->GetMomentsFeed(uid, last_moment_id, limit, moments, has_more);

            root["error"] = ErrorCodes::Success;
            root["has_more"] = has_more;
            memochat::json::JsonValue moments_arr(memochat::json::array_t{});
            for (const auto& moment : moments) {
                bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);
                memochat::json::JsonValue moment_json;
                moment_json["moment_id"] = static_cast<int64_t>(moment.moment_id);
                moment_json["uid"] = moment.uid;
                moment_json["visibility"] = moment.visibility;
                moment_json["location"] = moment.location;
                moment_json["created_at"] = static_cast<int64_t>(moment.created_at);
                moment_json["like_count"] = moment.like_count;
                moment_json["comment_count"] = moment.comment_count;
                moment_json["has_liked"] = has_liked;
                UserInfo uinfo;
                PostgresMgr::GetInstance()->GetUserInfo(moment.uid, uinfo);
                moment_json["user_id"] = uinfo.user_id;
                moment_json["user_name"] = uinfo.name;
                moment_json["user_nick"] = uinfo.nick;
                moment_json["user_icon"] = uinfo.icon;
                memochat::json::JsonValue items_arr(memochat::json::array_t{});
                for (const auto& item : content.items) {
                    memochat::json::JsonValue item_json;
                    item_json["seq"] = item.seq;
                    item_json["media_type"] = item.media_type;
                    item_json["media_key"] = item.media_key;
                    item_json["thumb_key"] = item.thumb_key;
                    item_json["content"] = item.content;
                    item_json["width"] = item.width;
                    item_json["height"] = item.height;
                    item_json["duration_ms"] = item.duration_ms;
                    items_arr.append(item_json);
                }
                moment_json["items"] = items_arr;
                moments_arr.append(moment_json);
            }
            root["moments"] = moments_arr;
            return true;
        });
    });

    logic.RegPost("/api/moments/detail", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            int64_t moment_id = src_root.get("moment_id", 0).asInt64();
            if (moment_id <= 0) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }

            MomentInfo moment;
            if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                root["error"] = ErrorCodes::RPCFailed;
                return false;
            }
            if (!PostgresMgr::GetInstance()->CanViewMoment(uid, moment)) {
                root["error"] = ErrorCodes::CallPermissionDenied;
                return false;
            }

            bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
            MomentContentInfo content;
            MongoMgr::GetInstance()->GetMomentContent(moment_id, content);
            memochat::json::JsonValue moment_json;
            moment_json["moment_id"] = static_cast<int64_t>(moment.moment_id);
            moment_json["uid"] = moment.uid;
            moment_json["visibility"] = moment.visibility;
            moment_json["location"] = moment.location;
            moment_json["created_at"] = static_cast<int64_t>(moment.created_at);
            moment_json["like_count"] = moment.like_count;
            moment_json["comment_count"] = moment.comment_count;
            moment_json["has_liked"] = has_liked;
            UserInfo uinfo;
            PostgresMgr::GetInstance()->GetUserInfo(moment.uid, uinfo);
            moment_json["user_id"] = uinfo.user_id;
            moment_json["user_name"] = uinfo.name;
            moment_json["user_nick"] = uinfo.nick;
            moment_json["user_icon"] = uinfo.icon;
            memochat::json::JsonValue items_arr(memochat::json::array_t{});
            for (const auto& item : content.items) {
                memochat::json::JsonValue item_json;
                item_json["media_type"] = item.media_type;
                item_json["media_key"] = item.media_key;
                item_json["content"] = item.content;
                items_arr.append(item_json);
            }
            moment_json["items"] = items_arr;
            root["error"] = ErrorCodes::Success;
            root["moment"] = moment_json;
            return true;
        });
    });

    logic.RegPost("/api/moments/delete", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            int64_t moment_id = src_root.get("moment_id", 0).asInt64();
            if (moment_id <= 0) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid);
            root["error"] = ErrorCodes::Success;
            return true;
        });
    });

    logic.RegPost("/api/moments/like", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            int64_t moment_id = src_root.get("moment_id", 0).asInt64();
            bool like = src_root.get("like", true).asBool();
            if (moment_id <= 0) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            bool ok = like
                ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);
            root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
            root["moment_id"] = static_cast<int64_t>(moment_id);
            if (ok) {
                MomentInfo moment;
                PostgresMgr::GetInstance()->GetMomentById(moment_id, moment);
                root["has_liked"] = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
                root["like_count"] = moment.like_count;
            }
            return true;
        });
    });

    logic.RegPost("/api/moments/comment", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            int64_t moment_id = src_root.get("moment_id", 0).asInt64();
            int64_t comment_id = src_root.get("comment_id", 0).asInt64();
            bool delete_mode = src_root.get("delete", false).asBool();
            std::string content_str = src_root.get("content", "").asString();
            delete_mode = delete_mode || (comment_id > 0 && content_str.empty());

            if (delete_mode) {
                if (comment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                }
                else {
                    bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                    root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                    root["moment_id"] = static_cast<int64_t>(moment_id);
                    root["delete"] = true;
                    MomentInfo moment;
                    if (moment_id > 0 && PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                        root["comment_count"] = moment.comment_count;
                    }
                }
                return true;
            }

            int reply_uid = src_root.get("reply_uid", 0).asInt();
            if (moment_id <= 0 || content_str.empty()) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }

            MomentCommentInfo comment;
            comment.moment_id = moment_id;
            comment.uid = uid;
            comment.content = content_str;
            comment.reply_uid = reply_uid;
            comment.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
            root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
            root["moment_id"] = static_cast<int64_t>(moment_id);
            root["delete"] = false;
            MomentInfo moment;
            if (PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                root["comment_count"] = moment.comment_count;
            }
            return true;
        });
    });

    logic.RegPost("/api/moments/comment/list", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            int uid = 0;
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }
            uid = src_root["uid"].asInt();
            int64_t moment_id = src_root.get("moment_id", 0).asInt64();
            int64_t last_comment_id = src_root.get("last_comment_id", 0).asInt64();
            int limit = src_root.get("limit", 20).asInt();
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;
            if (moment_id <= 0) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }

            std::vector<MomentCommentInfo> comments;
            bool has_more = false;
            PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more);
            root["error"] = ErrorCodes::Success;
            root["moment_id"] = static_cast<int64_t>(moment_id);
            root["has_more"] = has_more;
            MomentInfo moment;
            if (PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                root["comment_count"] = moment.comment_count;
            }
            memochat::json::JsonValue comments_arr(memochat::json::array_t{});
            for (const auto& comment : comments) {
                memochat::json::JsonValue comment_json;
                comment_json["id"] = static_cast<int64_t>(comment.id);
                comment_json["moment_id"] = static_cast<int64_t>(comment.moment_id);
                comment_json["uid"] = comment.uid;
                comment_json["content"] = comment.content;
                comment_json["reply_uid"] = comment.reply_uid;
                comment_json["created_at"] = static_cast<int64_t>(comment.created_at);
                UserInfo uinfo;
                PostgresMgr::GetInstance()->GetUserInfo(comment.uid, uinfo);
                comment_json["user_id"] = uinfo.user_id;
                comment_json["user_name"] = uinfo.name;
                comment_json["user_nick"] = uinfo.nick;
                comment_json["user_icon"] = uinfo.icon;
                if (comment.reply_uid > 0) {
                    UserInfo reply_user;
                    if (PostgresMgr::GetInstance()->GetUserInfo(comment.reply_uid, reply_user)) {
                        comment_json["reply_nick"] = reply_user.nick;
                    }
                }
                FillCommentLikeJson(uid, comment.id, comment_json);
                comments_arr.append(comment_json);
            }
            root["comments"] = comments_arr;
            return true;
        });
    });

    logic.RegPost("/api/moments/comment/like", [](std::shared_ptr<GateHttp3Connection> connection) {
        return GateHttp3JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string&) {
            if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }

            const int uid = src_root["uid"].asInt();
            const int64_t comment_id = src_root.get("comment_id", 0).asInt64();
            const bool like = src_root.get("like", true).asBool();
            if (uid <= 0 || comment_id <= 0) {
                root["error"] = ErrorCodes::Error_Json;
                return false;
            }

            const bool ok = like
                ? PostgresMgr::GetInstance()->AddMomentCommentLike(comment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentCommentLike(comment_id, uid);
            if (!ok) {
                root["error"] = ErrorCodes::RPCFailed;
                return false;
            }

            root["error"] = ErrorCodes::Success;
            root["comment_id"] = static_cast<int64_t>(comment_id);
            FillCommentLikeJson(uid, comment_id, root);
            return true;
        });
    });
}

}  // namespace GateHttp3ServiceImpl

void GateHttp3Service::RegisterRoutes(LogicSystem& logic) {
    GateHttp3ServiceImpl::RegisterRoutes(logic);
}



