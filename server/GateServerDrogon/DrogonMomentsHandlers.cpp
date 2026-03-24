#include "WinCompat.h"
#include "DrogonMomentsSupport.h"
#include "../GateServerCore/PostgresMgr.h"
#include "../GateServerCore/MongoMgr.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <chrono>

using namespace drogon;

namespace {

constexpr int kSuccess = 0;
constexpr int kErrorJson = 1001;
constexpr int kUidInvalid = 1011;
constexpr int kRpcFailed = 1002;
constexpr int kPermissionDenied = 1018;

Json::Value ToJson(const drogon::HttpRequestPtr& req) {
    Json::Value root;
    if (req->getContentType() == ContentType::CT_APPLICATION_JSON) {
        auto body = req->getBody();
        Json::Reader reader;
        reader.parse(std::string(body.data(), body.size()), root);
    }
    return root;
}

bool ValidateAuth(const Json::Value& src, Json::Value& resp, int& uid) {
    if (!src.isMember("uid") || !src.isMember("login_ticket")) {
        resp["error"] = kErrorJson;
        return false;
    }
    uid = src["uid"].asInt();
    if (uid <= 0) {
        resp["error"] = kUidInvalid;
        return false;
    }
    return true;
}

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void BuildUserProfile(int uid, Json::Value& json) {
    UserInfo info;
    if (PostgresMgr::GetInstance()->GetUserInfo(uid, info)) {
        json["user_name"] = info.name;
        json["user_nick"] = info.nick;
        json["user_icon"] = info.icon;
    }
    else {
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

void BuildMomentJson(const MomentInfo& moment, bool has_liked, const MomentContentInfo* content, Json::Value& json) {
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
        Json::Value items(Json::arrayValue);
        for (const auto& item : content->items) {
            Json::Value item_json;
            BuildMomentItem(item, item_json);
            items.append(item_json);
        }
        json["items"] = items;
    }
    else {
        json["items"] = Json::arrayValue;
    }
}

}  // namespace

void RegisterMomentsDrogonRoutes() {
    // POST /api/moments/publish
    app().registerHandler("/api/moments/publish",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int visibility = src.get("visibility", 0).asInt();
            std::string location = src.get("location", "").asString();
            int64_t now_ms = NowMs();

            MomentInfo moment;
            moment.uid = uid;
            moment.visibility = visibility;
            moment.location = location;
            moment.created_at = now_ms;

            if (!PostgresMgr::GetInstance()->AddMoment(moment)) {
                root["error"] = kRpcFailed;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t moment_id = now_ms;
            {
                std::vector<MomentInfo> moments;
                bool has_more = false;
                PostgresMgr::GetInstance()->GetMomentsFeed(uid, 0, 1, moments, has_more);
                for (const auto& m : moments) {
                    if (m.uid == uid && m.created_at == now_ms) {
                        moment_id = m.moment_id;
                        break;
                    }
                }
            }

            MomentContentInfo content;
            content.moment_id = moment_id;
            content.server_time = now_ms;
            if (src.isMember("items") && src["items"].isArray()) {
                for (const auto& item_json : src["items"]) {
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

            root["error"] = kSuccess;
            root["moment_id"] = static_cast<Json::Int64>(moment_id);
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});

    // POST /api/moments/list
    app().registerHandler("/api/moments/list",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t last_moment_id = src.get("last_moment_id", 0).asInt64();
            int limit = src.get("limit", 20).asInt();
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;

            std::vector<MomentInfo> moments;
            bool has_more = false;
            PostgresMgr::GetInstance()->GetMomentsFeed(uid, last_moment_id, limit, moments, has_more);

            root["error"] = kSuccess;
            root["has_more"] = has_more;
            Json::Value moments_arr(Json::arrayValue);
            for (const auto& moment : moments) {
                bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);
                Json::Value moment_json;
                BuildMomentJson(moment, has_liked, &content, moment_json);
                moments_arr.append(moment_json);
            }
            root["moments"] = moments_arr;
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});

    // POST /api/moments/detail
    app().registerHandler("/api/moments/detail",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t moment_id = src.get("moment_id", 0).asInt64();
            if (moment_id <= 0) {
                root["error"] = kErrorJson;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            MomentInfo moment;
            if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                root["error"] = kRpcFailed;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            if (moment.uid != uid && moment.visibility == 2) {
                root["error"] = kPermissionDenied;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
            MomentContentInfo content;
            MongoMgr::GetInstance()->GetMomentContent(moment_id, content);
            Json::Value moment_json;
            BuildMomentJson(moment, has_liked, &content, moment_json);
            root["error"] = kSuccess;
            root["moment"] = moment_json;
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});

    // POST /api/moments/delete
    app().registerHandler("/api/moments/delete",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t moment_id = src.get("moment_id", 0).asInt64();
            if (moment_id <= 0) {
                root["error"] = kErrorJson;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid);
            root["error"] = kSuccess;
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});

    // POST /api/moments/like
    app().registerHandler("/api/moments/like",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t moment_id = src.get("moment_id", 0).asInt64();
            bool like = src.get("like", true).asBool();
            if (moment_id <= 0) {
                root["error"] = kErrorJson;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            bool ok = like
                ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);

            root["error"] = ok ? kSuccess : kRpcFailed;
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});

    // POST /api/moments/comment
    app().registerHandler("/api/moments/comment",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t moment_id = src.get("moment_id", 0).asInt64();
            int64_t comment_id = src.get("comment_id", 0).asInt64();
            bool delete_mode = src.get("delete", false).asBool();

            if (delete_mode) {
                if (comment_id <= 0) {
                    root["error"] = kErrorJson;
                }
                else {
                    bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                    root["error"] = ok ? kSuccess : kRpcFailed;
                }
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            std::string content_str = src.get("content", "").asString();
            int reply_uid = src.get("reply_uid", 0).asInt();
            if (moment_id <= 0 || content_str.empty()) {
                root["error"] = kErrorJson;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            MomentCommentInfo comment;
            comment.moment_id = moment_id;
            comment.uid = uid;
            comment.content = content_str;
            comment.reply_uid = reply_uid;
            comment.created_at = NowMs();

            bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
            root["error"] = ok ? kSuccess : kRpcFailed;
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});

    // POST /api/moments/comment/list
    app().registerHandler("/api/moments/comment/list",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto src = ToJson(req);
            Json::Value root;
            int uid = 0;
            if (!ValidateAuth(src, root, uid)) {
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            int64_t moment_id = src.get("moment_id", 0).asInt64();
            int64_t last_comment_id = src.get("last_comment_id", 0).asInt64();
            int limit = src.get("limit", 20).asInt();
            if (limit <= 0) limit = 20;
            if (limit > 50) limit = 50;

            if (moment_id <= 0) {
                root["error"] = kErrorJson;
                auto resp = HttpResponse::newHttpJsonResponse(root);
                resp->setStatusCode(k200OK);
                callback(resp);
                return;
            }

            std::vector<MomentCommentInfo> comments;
            bool has_more = false;
            PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more);

            root["error"] = kSuccess;
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
            auto resp = HttpResponse::newHttpJsonResponse(root);
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Post, "NoFilter"});
}
