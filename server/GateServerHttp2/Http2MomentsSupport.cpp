#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2MomentsSupport.h"
#include "../GateServerCore/Http2MediaSupport.h"
#include "../GateServerCore/MongoMgr.h"
#include "../GateServerCore/PostgresMgr.h"
#include "../GateServerCore/const.h"
#include "logging/TraceContext.h"

#include <chrono>
#include <sstream>
#include <vector>

namespace {

int64_t NowMs() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

bool ValidateAuth(const memochat::json::JsonValue& src_root, int& uid) {
    if (!memochat::json::glaze_has_key(src_root, "uid") || !memochat::json::glaze_has_key(src_root, "login_ticket")) {
        return false;
    }
    uid = memochat::json::glaze_safe_get<int>(src_root, "uid", 0);
    return uid > 0;
}

void BuildUserProfile(int uid, memochat::json::JsonValue& json) {
    ::UserInfo user_info;
    if (PostgresMgr::GetInstance()->GetUserInfo(uid, user_info)) {
        json["user_name"] = user_info.name;
        json["user_nick"] = user_info.nick;
        json["user_icon"] = user_info.icon;
    } else {
        json["user_name"] = std::string();
        json["user_nick"] = std::string();
        json["user_icon"] = std::string();
    }
}

void BuildMomentItem(const MomentItemInfo& item, memochat::json::JsonValue& json) {
    json["seq"] = static_cast<double>(item.seq);
    json["media_type"] = item.media_type;
    json["media_key"] = item.media_key;
    json["thumb_key"] = item.thumb_key;
    json["content"] = item.content;
    json["width"] = static_cast<double>(item.width);
    json["height"] = static_cast<double>(item.height);
    json["duration_ms"] = static_cast<double>(item.duration_ms);
}

void BuildMomentJson(const MomentInfo& moment, bool has_liked,
                    const MomentContentInfo* content, memochat::json::JsonValue& json) {
    json["moment_id"] = static_cast<double>(moment.moment_id);
    json["uid"] = static_cast<double>(moment.uid);
    json["visibility"] = static_cast<double>(moment.visibility);
    json["location"] = moment.location;
    json["created_at"] = static_cast<double>(moment.created_at);
    json["like_count"] = static_cast<double>(moment.like_count);
    json["comment_count"] = static_cast<double>(moment.comment_count);
    json["has_liked"] = has_liked;
    BuildUserProfile(moment.uid, json);
    if (content) {
        memochat::json::JsonValue items_json = memochat::json::glaze_make_array();
        for (const auto& item : content->items) {
            memochat::json::JsonValue item_json;
            BuildMomentItem(item, item_json);
            memochat::json::glaze_append(items_json, item_json);
        }
        json["items"] = items_json;
    } else {
        json["items"] = memochat::json::glaze_make_array();
    }
}

memochat::json::JsonValue MakeMomentsError(int error_code, const std::string& message) {
    memochat::json::JsonValue resp;
    resp["error"] = static_cast<double>(error_code);
    if (!message.empty()) resp["message"] = message;
    return resp;
}

memochat::json::JsonValue MakeMomentsOk(const memochat::json::JsonValue& data) {
    memochat::json::JsonValue resp;
    resp["error"] = 0.0;
    if (memochat::json::glaze_is_object(data)) {
        for (const auto& key : memochat::json::getMemberNames(data)) {
            resp[key] = memochat::json::glaze_get(data, key);
        }
    }
    return resp;
}

}  // namespace

void RegisterHttp2MomentsRoutes() {
    Http2Routes::RegisterHandler("POST", "/api/moments/publish",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int visibility = memochat::json::glaze_safe_get<int>(root, "visibility", 0);
            std::string location = memochat::json::glaze_safe_get<std::string>(root, "location", std::string());
            auto now_ms = NowMs();

            MomentInfo moment;
            moment.uid = uid;
            moment.visibility = visibility;
            moment.location = location;
            moment.created_at = now_ms;
            moment.like_count = 0;
            moment.comment_count = 0;

            if (!PostgresMgr::GetInstance()->AddMoment(moment)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "add moment failed")));
                return;
            }

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

            MomentContentInfo content;
            content.moment_id = moment_id;
            content.server_time = now_ms;
            if (memochat::json::glaze_has_key(root, "items")) {
                const auto& items_val = memochat::json::glaze_get(root, "items");
                if (memochat::json::glaze_is_array(items_val)) {
                    const auto* arr = memochat::json::glaze_get_array(items_val);
                    if (arr) {
                        for (size_t idx = 0; idx < arr->size(); ++idx) {
                            const auto& item_json = memochat::json::glaze_get(arr, static_cast<int>(idx));
                                                    MomentItemInfo item;
                                                    item.media_type = memochat::json::glaze_safe_get<std::string>(item_json, "media_type", std::string("text"));
                        item.media_key = memochat::json::glaze_safe_get<std::string>(item_json, "media_key", std::string());
                        item.thumb_key = memochat::json::glaze_safe_get<std::string>(item_json, "thumb_key", std::string());
                        item.content = memochat::json::glaze_safe_get<std::string>(item_json, "content", std::string());
                        item.width = memochat::json::glaze_safe_get<int>(item_json, "width", 0);
                        item.height = memochat::json::glaze_safe_get<int>(item_json, "height", 0);
                        item.duration_ms = memochat::json::glaze_safe_get<int>(item_json, "duration_ms", 0);
                        content.items.push_back(item);
                        }
                    }
                }
            }
            if (moment_id > 0) {
                MongoMgr::GetInstance()->InsertMomentContent(content);
            }

            memochat::json::JsonValue data;
            data["moment_id"] = static_cast<double>(moment_id);
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(data)));
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/list",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int64_t last_moment_id = memochat::json::glaze_safe_get<int64_t>(root, "last_moment_id", 0LL);
            int limit = memochat::json::glaze_safe_get<int>(root, "limit", 20);
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;

            std::vector<MomentInfo> moments;
            bool has_more = false;
            if (!PostgresMgr::GetInstance()->GetMomentsFeed(uid, last_moment_id, limit, moments, has_more)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "get moments feed failed")));
                return;
            }

            memochat::json::JsonValue data;
            data["has_more"] = has_more;
            data["moments"] = memochat::json::glaze_make_array();
            for (const auto& moment : moments) {
                bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);
                memochat::json::JsonValue moment_json;
                BuildMomentJson(moment, has_liked, &content, moment_json);
                memochat::json::glaze_append(data["moments"], moment_json);
            }
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(data)));
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/detail",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(root, "moment_id", 0LL);
            if (moment_id <= 0) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id")));
                return;
            }

            MomentInfo moment;
            if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "moment not found")));
                return;
            }

            if (moment.uid != uid && moment.visibility == 2) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::CallPermissionDenied, "permission denied")));
                return;
            }

            bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
            MomentContentInfo content;
            MongoMgr::GetInstance()->GetMomentContent(moment_id, content);

            memochat::json::JsonValue data;
            memochat::json::JsonValue moment_json;
            BuildMomentJson(moment, has_liked, &content, moment_json);
            data["moment"] = moment_json;
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(data)));
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/delete",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(root, "moment_id", 0LL);
            if (moment_id <= 0) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id")));
                return;
            }

            if (!PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "delete failed")));
                return;
            }
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(memochat::json::JsonValue())));
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/like",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(root, "moment_id", 0LL);
            bool like = memochat::json::glaze_safe_get<bool>(root, "like", true);
            if (moment_id <= 0) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id")));
                return;
            }

            bool ok = like
                ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);
            if (!ok) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "like operation failed")));
                return;
            }
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(memochat::json::JsonValue())));
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/comment",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(root, "moment_id", 0LL);
            std::string content_str = memochat::json::glaze_safe_get<std::string>(root, "content", std::string());
            int reply_uid = memochat::json::glaze_safe_get<int>(root, "reply_uid", 0);
            int64_t comment_id = memochat::json::glaze_safe_get<int64_t>(root, "comment_id", 0LL);
            bool delete_mode = memochat::json::glaze_safe_get<bool>(root, "delete", false);

            if (delete_mode) {
                if (comment_id <= 0) {
                    resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid comment_id")));
                    return;
                }
                bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                resp.SetJsonBody(ok ? memochat::json::glaze_stringify(MakeMomentsOk(memochat::json::JsonValue()))
                                    : memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "delete failed")));
                return;
            }

            if (moment_id <= 0 || content_str.empty()) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid params")));
                return;
            }

            MomentCommentInfo comment;
            comment.moment_id = moment_id;
            comment.uid = uid;
            comment.content = content_str;
            comment.reply_uid = reply_uid;
            comment.created_at = NowMs();

            bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
            if (!ok) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "add comment failed")));
                return;
            }
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(memochat::json::JsonValue())));
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/comment/list",
        [](const Http2Request& req, Http2Response& resp) {
            memochat::json::JsonValue root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(1, "invalid json")));
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params")));
                return;
            }

            int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(root, "moment_id", 0LL);
            int64_t last_comment_id = memochat::json::glaze_safe_get<int64_t>(root, "last_comment_id", 0LL);
            int limit = memochat::json::glaze_safe_get<int>(root, "limit", 20);
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;
            if (moment_id <= 0) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id")));
                return;
            }

            std::vector<MomentCommentInfo> comments;
            bool has_more = false;
            if (!PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more)) {
                resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsError(ErrorCodes::RPCFailed, "get comments failed")));
                return;
            }

            memochat::json::JsonValue data;
            data["has_more"] = has_more;
            data["comments"] = memochat::json::glaze_make_array();
            for (const auto& comment : comments) {
                memochat::json::JsonValue comment_json;
                comment_json["id"] = static_cast<double>(comment.id);
                comment_json["moment_id"] = static_cast<double>(comment.moment_id);
                comment_json["uid"] = static_cast<double>(comment.uid);
                comment_json["content"] = comment.content;
                comment_json["reply_uid"] = static_cast<double>(comment.reply_uid);
                comment_json["created_at"] = static_cast<double>(comment.created_at);
                BuildUserProfile(comment.uid, comment_json);
                if (comment.reply_uid > 0) {
                    ::UserInfo reply_user;
                    if (PostgresMgr::GetInstance()->GetUserInfo(comment.reply_uid, reply_user)) {
                        comment_json["reply_nick"] = reply_user.nick;
                    }
                }
                memochat::json::glaze_append(data["comments"], comment_json);
            }
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMomentsOk(data)));
        });
}