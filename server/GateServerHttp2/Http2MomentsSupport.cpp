#include "WinCompat.h"
#include "Http2MomentsSupport.h"
#include "../GateServerCore/Http2MediaSupport.h"
#include "../GateServerCore/MongoMgr.h"
#include "../GateServerCore/PostgresMgr.h"
#include "../GateServerCore/const.h"
#include "logging/TraceContext.h"

#include <chrono>
#include <json/json.h>
#include <sstream>
#include <vector>

namespace {

int64_t NowMs() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

bool ValidateAuth(const Json::Value& src_root, int& uid) {
    if (!src_root.isMember("uid") || !src_root.isMember("login_ticket")) {
        return false;
    }
    uid = src_root["uid"].asInt();
    return uid > 0;
}

void BuildUserProfile(int uid, Json::Value& json) {
    ::UserInfo user_info;
    if (PostgresMgr::GetInstance()->GetUserInfo(uid, user_info)) {
        json["user_name"] = user_info.name;
        json["user_nick"] = user_info.nick;
        json["user_icon"] = user_info.icon;
    } else {
        json["user_name"] = "";
        json["user_nick"] = "";
        json["user_icon"] = "";
    }
}

void BuildMomentItem(const MomentItemInfo& item, Json::Value& json) {
    json["seq"] = item.seq;
    json["media_type"] = item.media_type;
    json["media_key"] = item.media_key;
    json["thumb_key"] = item.thumb_key;
    json["content"] = item.content;
    json["width"] = item.width;
    json["height"] = item.height;
    json["duration_ms"] = item.duration_ms;
}

void BuildMomentJson(const MomentInfo& moment, bool has_liked,
                    const MomentContentInfo* content, Json::Value& json) {
    json["moment_id"] = static_cast<Json::Int64>(moment.moment_id);
    json["uid"] = moment.uid;
    json["visibility"] = moment.visibility;
    json["location"] = moment.location;
    json["created_at"] = static_cast<Json::Int64>(moment.created_at);
    json["like_count"] = moment.like_count;
    json["comment_count"] = moment.comment_count;
    json["has_liked"] = has_liked;
    BuildUserProfile(moment.uid, json);
    if (content) {
        Json::Value items_json(Json::arrayValue);
        for (const auto& item : content->items) {
            Json::Value item_json;
            BuildMomentItem(item, item_json);
            items_json.append(item_json);
        }
        json["items"] = items_json;
    } else {
        json["items"] = Json::arrayValue;
    }
}

Json::Value MakeMomentsError(int error_code, const std::string& message) {
    Json::Value resp;
    resp["error"] = error_code;
    if (!message.empty()) resp["message"] = message;
    return resp;
}

Json::Value MakeMomentsOk(const Json::Value& data) {
    Json::Value resp;
    resp["error"] = ErrorCodes::Success;
    if (data.isObject()) {
        for (const auto& key : data.getMemberNames()) {
            resp[key] = data[key];
        }
    }
    return resp;
}

}  // namespace

void RegisterHttp2MomentsRoutes() {
    Http2Routes::RegisterHandler("POST", "/api/moments/publish",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int visibility = root.get("visibility", 0).asInt();
            std::string location = root.get("location", "").asString();
            auto now_ms = NowMs();

            MomentInfo moment;
            moment.uid = uid;
            moment.visibility = visibility;
            moment.location = location;
            moment.created_at = now_ms;
            moment.like_count = 0;
            moment.comment_count = 0;

            if (!PostgresMgr::GetInstance()->AddMoment(moment)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "add moment failed").toStyledString());
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
            if (root.isMember("items") && root["items"].isArray()) {
                for (const auto& item_json : root["items"]) {
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

            Json::Value data;
            data["moment_id"] = static_cast<Json::Int64>(moment_id);
            resp.SetJsonBody(MakeMomentsOk(data).toStyledString());
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/list",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int64_t last_moment_id = root.get("last_moment_id", 0).asInt64();
            int limit = root.get("limit", 20).asInt();
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;

            std::vector<MomentInfo> moments;
            bool has_more = false;
            if (!PostgresMgr::GetInstance()->GetMomentsFeed(uid, last_moment_id, limit, moments, has_more)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "get moments feed failed").toStyledString());
                return;
            }

            Json::Value data;
            data["has_more"] = has_more;
            data["moments"] = Json::arrayValue;
            for (const auto& moment : moments) {
                bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);
                Json::Value moment_json;
                BuildMomentJson(moment, has_liked, &content, moment_json);
                data["moments"].append(moment_json);
            }
            resp.SetJsonBody(MakeMomentsOk(data).toStyledString());
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/detail",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int64_t moment_id = root.get("moment_id", 0).asInt64();
            if (moment_id <= 0) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id").toStyledString());
                return;
            }

            MomentInfo moment;
            if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "moment not found").toStyledString());
                return;
            }

            if (moment.uid != uid && moment.visibility == 2) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::CallPermissionDenied, "permission denied").toStyledString());
                return;
            }

            bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
            MomentContentInfo content;
            MongoMgr::GetInstance()->GetMomentContent(moment_id, content);

            Json::Value data;
            BuildMomentJson(moment, has_liked, &content, data["moment"]);
            resp.SetJsonBody(MakeMomentsOk(data).toStyledString());
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/delete",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int64_t moment_id = root.get("moment_id", 0).asInt64();
            if (moment_id <= 0) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id").toStyledString());
                return;
            }

            if (!PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "delete failed").toStyledString());
                return;
            }
            resp.SetJsonBody(MakeMomentsOk(Json::objectValue).toStyledString());
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/like",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int64_t moment_id = root.get("moment_id", 0).asInt64();
            bool like = root.get("like", true).asBool();
            if (moment_id <= 0) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id").toStyledString());
                return;
            }

            bool ok = like
                ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);
            if (!ok) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "like operation failed").toStyledString());
                return;
            }
            resp.SetJsonBody(MakeMomentsOk(Json::objectValue).toStyledString());
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/comment",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int64_t moment_id = root.get("moment_id", 0).asInt64();
            std::string content_str = root.get("content", "").asString();
            int reply_uid = root.get("reply_uid", 0).asInt();
            int64_t comment_id = root.get("comment_id", 0).asInt64();
            bool delete_mode = root.get("delete", false).asBool();

            if (delete_mode) {
                if (comment_id <= 0) {
                    resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid comment_id").toStyledString());
                    return;
                }
                bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                resp.SetJsonBody(ok ? MakeMomentsOk(Json::objectValue).toStyledString()
                                    : MakeMomentsError(ErrorCodes::RPCFailed, "delete failed").toStyledString());
                return;
            }

            if (moment_id <= 0 || content_str.empty()) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid params").toStyledString());
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
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "add comment failed").toStyledString());
                return;
            }
            resp.SetJsonBody(MakeMomentsOk(Json::objectValue).toStyledString());
        });

    Http2Routes::RegisterHandler("POST", "/api/moments/comment/list",
        [](const Http2Request& req, Http2Response& resp) {
            Json::Value root;
            if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
                resp.SetJsonBody(MakeMomentsError(1, "invalid json").toStyledString());
                return;
            }

            int uid = 0;
            if (!ValidateAuth(root, uid)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid auth params").toStyledString());
                return;
            }

            int64_t moment_id = root.get("moment_id", 0).asInt64();
            int64_t last_comment_id = root.get("last_comment_id", 0).asInt64();
            int limit = root.get("limit", 20).asInt();
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;
            if (moment_id <= 0) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::Error_Json, "invalid moment_id").toStyledString());
                return;
            }

            std::vector<MomentCommentInfo> comments;
            bool has_more = false;
            if (!PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more)) {
                resp.SetJsonBody(MakeMomentsError(ErrorCodes::RPCFailed, "get comments failed").toStyledString());
                return;
            }

            Json::Value data;
            data["has_more"] = has_more;
            data["comments"] = Json::arrayValue;
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
                    ::UserInfo reply_user;
                    if (PostgresMgr::GetInstance()->GetUserInfo(comment.reply_uid, reply_user)) {
                        comment_json["reply_nick"] = reply_user.nick;
                    }
                }
                data["comments"].append(comment_json);
            }
            resp.SetJsonBody(MakeMomentsOk(data).toStyledString());
        });
}
