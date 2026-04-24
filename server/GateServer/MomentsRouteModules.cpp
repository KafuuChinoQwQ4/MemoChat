#include "MomentsRouteModules.h"

#include "GateHttpJsonSupport.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "logging/TraceContext.h"

#include "logging/Logger.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = memochat::json;

namespace {

bool ValidateAuth(const json::JsonValue& src_root, json::JsonValue& root, int& uid) {
    if (!json::isMember(src_root, "uid") || !json::isMember(src_root, "login_ticket")) {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    if (uid <= 0) {
        root["error"] = ErrorCodes::UidInvalid;
        return false;
    }
    return true;
}

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void BuildMomentItem(const MomentItemInfo& item, json::JsonValue& json) {
    json["seq"] = item.seq;
    json["media_type"] = item.media_type;
    json["media_key"] = item.media_key;
    json["thumb_key"] = item.thumb_key;
    json["content"] = item.content;
    json["width"] = item.width;
    json["height"] = item.height;
    json["duration_ms"] = item.duration_ms;
}

void BuildUserProfile(int uid, json::JsonValue& json) {
    UserInfo user_info;
    if (PostgresMgr::GetInstance()->GetUserInfo(uid, user_info)) {
        json["user_name"] = user_info.name;
        json["user_nick"] = user_info.nick;
        json["user_icon"] = user_info.icon;
    }
    else {
        json["user_name"] = "";
        json["user_nick"] = "";
        json["user_icon"] = "";
    }
}

void BuildLikeList(const std::vector<MomentLikeInfo>& likes, json::JsonValue& json) {
    json = json::glaze_make_array();
    for (const auto& like : likes) {
        json::JsonValue item;
        item["uid"] = like.uid;
        item["user_nick"] = like.user_nick;
        item["user_icon"] = like.user_icon;
        item["created_at"] = static_cast<int64_t>(like.created_at);
        json::glaze_append(json, item);
    }
}

void BuildCommentList(const std::vector<MomentCommentInfo>& comments, json::JsonValue& json) {
    json = json::glaze_make_array();
    for (const auto& comment : comments) {
        json::JsonValue item;
        item["id"] = static_cast<int64_t>(comment.id);
        item["uid"] = comment.uid;
        item["user_nick"] = comment.user_nick;
        item["user_icon"] = comment.user_icon;
        item["content"] = comment.content;
        item["reply_uid"] = comment.reply_uid;
        item["reply_nick"] = comment.reply_nick;
        item["created_at"] = static_cast<int64_t>(comment.created_at);
        json::glaze_append(json, item);
    }
}

void BuildMomentJson(const MomentInfo& moment, bool has_liked, const MomentContentInfo* content,
                      const std::vector<MomentLikeInfo>* likes,
                      const std::vector<MomentCommentInfo>* comments,
                      json::JsonValue& json) {
    json["moment_id"] = static_cast<int64_t>(moment.moment_id);
    json["uid"] = moment.uid;
    json["visibility"] = moment.visibility;
    json["location"] = moment.location;
    json["created_at"] = static_cast<int64_t>(moment.created_at);
    json["like_count"] = moment.like_count;
    json["comment_count"] = moment.comment_count;
    json["has_liked"] = has_liked;
    BuildUserProfile(moment.uid, json);

    if (content) {
        json::JsonValue items_json = json::glaze_make_array();
        for (const auto& item : content->items) {
            json::JsonValue item_json;
            BuildMomentItem(item, item_json);
            json::glaze_append(items_json, item_json);
        }
        json["items"] = items_json;
    }
    else {
        json["items"] = json::glaze_make_array();
    }

    if (likes) {
        json::JsonValue like_names = json::glaze_make_array();
        for (const auto& like : *likes) {
            if (!like.user_nick.empty()) {
                json::glaze_append(like_names, like.user_nick);
            }
        }
        json["like_names"] = like_names;
        json::JsonValue likes_arr = json::glaze_make_array();
        BuildLikeList(*likes, likes_arr);
        json["likes"] = likes_arr;
    }
    else {
        json["like_names"] = json::glaze_make_array();
        json["likes"] = json::glaze_make_array();
    }

    if (comments) {
        json::JsonValue comments_arr = json::glaze_make_array();
        BuildCommentList(*comments, comments_arr);
        json["comments"] = comments_arr;
    }
    else {
        json["comments"] = json::glaze_make_array();
    }
}

}  // namespace

void MomentsHttpServiceRoutes::RegisterRoutes(LogicSystem& logic) {

    // POST /api/moments/publish
    logic.RegPost("/api/moments/publish", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                int visibility = src_root.get("visibility", 0).asInt();
                std::string location = src_root.get("location", "").asString();
                auto now_ms = NowMs();

                MomentInfo moment;
                moment.uid = uid;
                moment.visibility = visibility;
                moment.location = location;
                moment.created_at = now_ms;
                moment.like_count = 0;
                moment.comment_count = 0;

                if (!PostgresMgr::GetInstance()->AddMoment(moment)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                // Get the auto-generated moment_id by querying
                std::vector<MomentInfo> moments;
                bool has_more = false;
                PostgresMgr::GetInstance()->GetMomentsFeed(uid, 0, 1, moments, has_more);
                int64_t moment_id = 0;
                for (const auto& m : moments) {
                    if (m.uid == uid && m.created_at == now_ms) {
                        moment_id = m.moment_id;
                        break;
                    }
                }

                // Insert content into MongoDB
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
                        content.items.push_back(item);
                    }
                }

                if (moment_id > 0) {
                    MongoMgr::GetInstance()->InsertMomentContent(content);
                }

                root["error"] = ErrorCodes::Success;
                root["moment_id"] = static_cast<Json::Int64>(moment_id);
                return true;
            });
    });

    // POST /api/moments/list
    logic.RegPost("/api/moments/list", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                int64_t last_moment_id = src_root.get("last_moment_id", 0).asInt64();
                int limit = src_root.get("limit", 20).asInt();
                if (limit <= 0) limit = 20;
                if (limit > 50) limit = 50;

                std::vector<MomentInfo> moments;
                bool has_more = false;
                if (!PostgresMgr::GetInstance()->GetMomentsFeed(uid, last_moment_id, limit, moments, has_more)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                root["error"] = ErrorCodes::Success;
                root["has_more"] = has_more;

                Json::Value moments_arr(Json::arrayValue);
                for (const auto& moment : moments) {
                    bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                    MomentContentInfo content;
                    MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);

                    Json::Value moment_json;
                    BuildMomentJson(moment, has_liked, &content, nullptr, nullptr, moment_json);
                    moments_arr.append(moment_json);
                }
                root["moments"] = moments_arr;
                return true;
            });
    });

    // POST /api/moments/detail
    logic.RegPost("/api/moments/detail", [](std::shared_ptr<HttpConnection> connection) {
        memolog::LogInfo("gate.moments.detail", "detail handler entered");
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    memolog::LogWarn("gate.moments.detail", "auth failed");
                    return true;
                }

                int64_t moment_id = src_root.get("moment_id", 0).asInt64();
                memolog::LogInfo("gate.moments.detail", "moment_id=" + std::to_string(moment_id));
                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    memolog::LogWarn("gate.moments.detail", "invalid moment_id");
                    return true;
                }

                MomentInfo moment;
                if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    memolog::LogWarn("gate.moments.detail", "GetMomentById failed, moment_id=" + std::to_string(moment_id));
                    return true;
                }

                // Check visibility
                if (moment.uid != uid && moment.visibility == 2) {
                    root["error"] = ErrorCodes::CallPermissionDenied;
                    return true;
                }

                bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment_id, content);

                std::vector<MomentLikeInfo> likes;
                bool likes_has_more = false;
                PostgresMgr::GetInstance()->GetMomentLikes(moment_id, 100, likes, likes_has_more);

                std::vector<MomentCommentInfo> comments;
                bool comments_has_more = false;
                PostgresMgr::GetInstance()->GetMomentComments(moment_id, 0, 100, comments, comments_has_more);
                memolog::LogInfo("gate.moments.detail", "found comments=" + std::to_string(comments.size()));

                Json::Value moment_json;
                BuildMomentJson(moment, has_liked, &content, &likes, &comments, moment_json);
                root["error"] = ErrorCodes::Success;
                root["moment"] = moment_json;
                return true;
            });
    });

    // POST /api/moments/delete
    logic.RegPost("/api/moments/delete", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                int64_t moment_id = src_root.get("moment_id", 0).asInt64();
                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                if (!PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                root["error"] = ErrorCodes::Success;
                root["moment_id"] = static_cast<Json::Int64>(moment_id);
                return true;
            });
    });

    // POST /api/moments/like
    logic.RegPost("/api/moments/like", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                int64_t moment_id = src_root.get("moment_id", 0).asInt64();
                bool like = src_root.get("like", true).asBool();
                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                bool ok = like
                    ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                    : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);

                if (!ok) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                root["error"] = ErrorCodes::Success;
                return true;
            });
    });

    // POST /api/moments/comment
    logic.RegPost("/api/moments/comment", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                int64_t moment_id = src_root.get("moment_id", 0).asInt64();
                std::string content = src_root.get("content", "").asString();
                int reply_uid = src_root.get("reply_uid", 0).asInt();
                int64_t comment_id = src_root.get("comment_id", 0).asInt64();
                bool delete_mode = src_root.get("delete", false).asBool();

                if (delete_mode) {
                    if (comment_id <= 0) {
                        root["error"] = ErrorCodes::Error_Json;
                        return true;
                    }
                    bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                    root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                    root["moment_id"] = static_cast<Json::Int64>(moment_id);
                    root["delete"] = true;
                    return true;
                }

                if (moment_id <= 0 || content.empty()) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                MomentCommentInfo comment;
                comment.moment_id = moment_id;
                comment.uid = uid;
                comment.content = content;
                comment.reply_uid = reply_uid;
                comment.created_at = NowMs();

                bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
                root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                root["moment_id"] = static_cast<Json::Int64>(moment_id);
                root["delete"] = false;
                return true;
            });
    });

    // POST /api/moments/comment/list
    logic.RegPost("/api/moments/comment/list", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                int64_t moment_id = src_root.get("moment_id", 0).asInt64();
                int64_t last_comment_id = src_root.get("last_comment_id", 0).asInt64();
                int limit = src_root.get("limit", 20).asInt();
                if (limit <= 0) limit = 20;
                if (limit > 50) limit = 50;

                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                std::vector<MomentCommentInfo> comments;
                bool has_more = false;
                if (!PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                root["error"] = ErrorCodes::Success;
                root["has_more"] = has_more;

                Json::Value comments_arr(Json::arrayValue);
                for (const auto& comment : comments) {
                    Json::Value comment_json;
                    comment_json["id"] = static_cast<Json::Int64>(comment.id);
                    comment_json["moment_id"] = static_cast<Json::Int64>(comment.moment_id);
                    comment_json["uid"] = comment.uid;
                    comment_json["content"] = comment.content;
                    comment_json["reply_uid"] = comment.reply_uid;
                    comment_json["created_at"] = static_cast<Json::Int64>(comment.created_at);
                    BuildUserProfile(comment.uid, comment_json);
                    if (comment.reply_uid > 0) {
                        UserInfo reply_user;
                        if (PostgresMgr::GetInstance()->GetUserInfo(comment.reply_uid, reply_user)) {
                            comment_json["reply_nick"] = reply_user.nick;
                        }
                    }
                    comments_arr.append(comment_json);
                }
                root["comments"] = comments_arr;
                return true;
            });
    });
}
