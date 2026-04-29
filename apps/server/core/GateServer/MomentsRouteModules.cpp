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
#include <cstdlib>

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

json::JsonValue ReadMember(const json::JsonValue& obj, const char* key) {
    if (!json::isMember(obj, key)) {
        return json::JsonValue();
    }
    return obj[key];
}

int64_t ReadInt64(const json::JsonValue& obj, const char* key, int64_t fallback = 0) {
    const json::JsonValue value = ReadMember(obj, key);
    try {
        return value.asInt64();
    } catch (...) {
    }
    try {
        return static_cast<int64_t>(value.asInt());
    } catch (...) {
    }
    try {
        return static_cast<int64_t>(value.asDouble());
    } catch (...) {
    }
    const std::string text = value.asString();
    if (!text.empty()) {
        char* end = nullptr;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        if (end && *end == '\0') {
            return static_cast<int64_t>(parsed);
        }
    }
    return fallback;
}

int ReadInt(const json::JsonValue& obj, const char* key, int fallback = 0) {
    return static_cast<int>(ReadInt64(obj, key, fallback));
}

bool ReadBool(const json::JsonValue& obj, const char* key, bool fallback = false) {
    const json::JsonValue value = ReadMember(obj, key);
    try {
        return value.asBool();
    } catch (...) {
    }
    const std::string text = value.asString();
    if (text == "true" || text == "1") {
        return true;
    }
    if (text == "false" || text == "0") {
        return false;
    }
    try {
        return value.asInt() != 0;
    } catch (...) {
    }
    try {
        return value.asDouble() != 0.0;
    } catch (...) {
    }
    return fallback;
}

std::string ReadString(const json::JsonValue& obj, const char* key, const std::string& fallback = {}) {
    const json::JsonValue value = ReadMember(obj, key);
    const std::string text = value.asString();
    return text.empty() ? fallback : text;
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
        json["user_id"] = user_info.user_id;
        json["user_name"] = user_info.name;
        json["user_nick"] = user_info.nick;
        json["user_icon"] = user_info.icon;
    }
    else {
        json["user_id"] = "";
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
        item["moment_id"] = static_cast<int64_t>(comment.moment_id);
        item["uid"] = comment.uid;
        item["user_nick"] = comment.user_nick;
        item["user_icon"] = comment.user_icon;
        item["content"] = comment.content;
        item["reply_uid"] = comment.reply_uid;
        item["reply_nick"] = comment.reply_nick;
        item["created_at"] = static_cast<int64_t>(comment.created_at);
        item["like_count"] = comment.like_count;
        item["has_liked"] = comment.has_liked;
        json::JsonValue like_names = json::glaze_make_array();
        for (const auto& like : comment.likes) {
            if (!like.user_nick.empty()) {
                json::glaze_append(like_names, like.user_nick);
            }
        }
        item["like_names"] = like_names;
        json::JsonValue likes_arr = json::glaze_make_array();
        BuildLikeList(comment.likes, likes_arr);
        item["likes"] = likes_arr;
        json::glaze_append(json, item);
    }
}

void FillCommentLikes(int viewer_uid, std::vector<MomentCommentInfo>& comments) {
    for (auto& comment : comments) {
        bool has_more = false;
        std::vector<MomentLikeInfo> likes;
        if (PostgresMgr::GetInstance()->GetMomentCommentLikes(comment.id, 100, likes, has_more)) {
            comment.likes = std::move(likes);
            comment.like_count = static_cast<int>(comment.likes.size());
        }
        comment.has_liked = PostgresMgr::GetInstance()->HasLikedMomentComment(comment.id, viewer_uid);
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

                int visibility = ReadInt(src_root, "visibility", 0);
                std::string location = ReadString(src_root, "location");
                auto now_ms = NowMs();

                MomentInfo moment;
                moment.uid = uid;
                moment.visibility = visibility;
                moment.location = location;
                moment.created_at = now_ms;
                moment.like_count = 0;
                moment.comment_count = 0;

                int64_t moment_id = 0;
                if (!PostgresMgr::GetInstance()->AddMoment(moment, &moment_id)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
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
                        NormalizeMomentItem(item);
                        content.items.push_back(item);
                    }
                }
                const std::string raw_content = ReadString(src_root, "content");
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
                    memolog::LogError("gate.moments.publish", "failed to store moment content in MongoDB",
                        { {"moment_id", std::to_string(moment_id)}, {"uid", std::to_string(uid)} });
                    return true;
                }

                root["error"] = ErrorCodes::Success;
                root["moment_id"] = static_cast<Json::Int64>(moment_id);
                memolog::LogInfo("gate.moments.publish.ok", "moment published with content",
                    { {"moment_id", std::to_string(moment_id)},
                      {"uid", std::to_string(uid)},
                      {"items", std::to_string(content.items.size())} });
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

                int64_t last_moment_id = ReadInt64(src_root, "last_moment_id", 0);
                int limit = ReadInt(src_root, "limit", 20);
                int author_uid = ReadInt(src_root, "author_uid", 0);
                if (limit <= 0) limit = 20;
                if (limit > 50) limit = 50;

                std::vector<MomentInfo> moments;
                bool has_more = false;
                if (!PostgresMgr::GetInstance()->GetMomentsFeed(uid, last_moment_id, limit, author_uid, moments, has_more)) {
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

                int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
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
                if (!PostgresMgr::GetInstance()->CanViewMoment(uid, moment)) {
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
                FillCommentLikes(uid, comments);
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

                int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
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

                int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
                bool like = ReadBool(src_root, "like", true);
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

                MomentInfo moment;
                PostgresMgr::GetInstance()->GetMomentById(moment_id, moment);
                root["error"] = ErrorCodes::Success;
                root["moment_id"] = static_cast<Json::Int64>(moment_id);
                root["has_liked"] = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
                root["like_count"] = moment.like_count;
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

                int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
                std::string content = ReadString(src_root, "content");
                int reply_uid = ReadInt(src_root, "reply_uid", 0);
                int64_t comment_id = ReadInt64(src_root, "comment_id", 0);
                bool delete_mode = ReadBool(src_root, "delete", false)
                    || (comment_id > 0 && content.empty());

                if (delete_mode) {
                    if (comment_id <= 0) {
                        root["error"] = ErrorCodes::Error_Json;
                        return true;
                    }
                    bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                    root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                    root["moment_id"] = static_cast<Json::Int64>(moment_id);
                    root["delete"] = true;
                    MomentInfo moment;
                    if (moment_id > 0 && PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                        root["comment_count"] = moment.comment_count;
                    }
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
                MomentInfo moment;
                if (PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                    root["comment_count"] = moment.comment_count;
                }
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

                int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
                int64_t last_comment_id = ReadInt64(src_root, "last_comment_id", 0);
                int limit = ReadInt(src_root, "limit", 20);
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
                FillCommentLikes(uid, comments);

                root["error"] = ErrorCodes::Success;
                root["moment_id"] = static_cast<Json::Int64>(moment_id);
                root["has_more"] = has_more;
                MomentInfo moment;
                if (PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                    root["comment_count"] = moment.comment_count;
                }

                Json::Value comments_arr(Json::arrayValue);
                BuildCommentList(comments, comments_arr);
                root["comments"] = comments_arr;
                return true;
            });
    });

    // POST /api/moments/comment/like
    logic.RegPost("/api/moments/comment/like", [](std::shared_ptr<HttpConnection> connection) {
        return GateHttpJsonSupport::HandleJsonPost(connection,
            [](const Json::Value& src_root, Json::Value& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) {
                    return true;
                }

                const int64_t comment_id = ReadInt64(src_root, "comment_id", 0);
                const bool like = ReadBool(src_root, "like", true);
                if (comment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                const bool ok = like
                    ? PostgresMgr::GetInstance()->AddMomentCommentLike(comment_id, uid)
                    : PostgresMgr::GetInstance()->RemoveMomentCommentLike(comment_id, uid);
                if (!ok) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                std::vector<MomentLikeInfo> likes;
                bool has_more = false;
                PostgresMgr::GetInstance()->GetMomentCommentLikes(comment_id, 100, likes, has_more);

                root["error"] = ErrorCodes::Success;
                root["comment_id"] = static_cast<Json::Int64>(comment_id);
                root["has_liked"] = PostgresMgr::GetInstance()->HasLikedMomentComment(comment_id, uid);
                root["like_count"] = static_cast<int>(likes.size());
                Json::Value like_names(Json::arrayValue);
                for (const auto& item : likes) {
                    if (!item.user_nick.empty()) {
                        like_names.append(item.user_nick);
                    }
                }
                root["like_names"] = like_names;
                Json::Value likes_arr(Json::arrayValue);
                for (const auto& item : likes) {
                    Json::Value like_json;
                    like_json["uid"] = item.uid;
                    like_json["user_nick"] = item.user_nick;
                    like_json["user_icon"] = item.user_icon;
                    like_json["created_at"] = static_cast<Json::Int64>(item.created_at);
                    likes_arr.append(like_json);
                }
                root["likes"] = likes_arr;
                return true;
            });
    });
}
