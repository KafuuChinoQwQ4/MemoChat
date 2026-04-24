#include "H1AuthRoutes.h"
#include "json/GlazeCompat.h"

#include "H1JsonSupport.h"
#include "H1Connection.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "logging/TraceContext.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
namespace beast = boost::beast;
namespace http = beast::http;

namespace {

bool ValidateAuth(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, int& uid) {
    if (!memochat::json::glaze_has_key(src_root, "uid") || !memochat::json::glaze_has_key(src_root, "login_ticket")) {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    uid = memochat::json::glaze_safe_get<int>(src_root, "uid", 0);
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

void BuildMomentItem(const MomentItemInfo& item, memochat::json::JsonValue& json) {
    json["seq"] = item.seq;
    json["media_type"] = item.media_type;
    json["media_key"] = item.media_key;
    json["thumb_key"] = item.thumb_key;
    json["content"] = item.content;
    json["width"] = item.width;
    json["height"] = item.height;
    json["duration_ms"] = item.duration_ms;
}

void BuildUserProfile(int uid, memochat::json::JsonValue& json) {
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

void BuildMomentJson(const MomentInfo& moment, bool has_liked, const MomentContentInfo* content, memochat::json::JsonValue& json) {
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
        memochat::json::JsonValue items_json(memochat::json::JsonValue{});
        for (const auto& item : content->items) {
            memochat::json::JsonValue item_json;
            BuildMomentItem(item, item_json);
            memochat::json::glaze_append(items_json, item_json);
        }
        json["items"] = items_json;
    }
    else {
        json["items"] = memochat::json::JsonValue{};
    }
}

}  // namespace

void H1MomentsServiceRoutes::RegisterRoutes(H1LogicSystem& logic) {

    logic.RegPost("/api/moments/publish", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int visibility = memochat::json::glaze_safe_get<int>(src_root, "visibility", 0);
                std::string location = memochat::json::glaze_safe_get<std::string>(src_root, "location", "");
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

                if (memochat::json::glaze_has_key(src_root, "items") && memochat::json::glaze_is_array(src_root["items"])) {
                    const auto& arr = src_root["items"].get<memochat::json::array_t>();
                    for (size_t idx = 0; idx < arr.size(); ++idx) {
                        const auto& item_json = arr[idx];
                        MomentItemInfo item;
                        item.media_type = memochat::json::glaze_safe_get<std::string>(item_json, "media_type", "text");
                        item.media_key = memochat::json::glaze_safe_get<std::string>(item_json, "media_key", "");
                        item.thumb_key = memochat::json::glaze_safe_get<std::string>(item_json, "thumb_key", "");
                        item.content = memochat::json::glaze_safe_get<std::string>(item_json, "content", "");
                        item.width = memochat::json::glaze_safe_get<int>(item_json, "width", 0);
                        item.height = memochat::json::glaze_safe_get<int>(item_json, "height", 0);
                        item.duration_ms = memochat::json::glaze_safe_get<int>(item_json, "duration_ms", 0);
                        content.items.push_back(item);
                    }
                }

                if (moment_id > 0) {
                    MongoMgr::GetInstance()->InsertMomentContent(content);
                }

                root["error"] = ErrorCodes::Success;
                root["moment_id"] = static_cast<int64_t>(moment_id);
                return true;
            });
    });

    logic.RegPost("/api/moments/list", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int64_t last_moment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "last_moment_id", 0LL);
                int limit = memochat::json::glaze_safe_get<int>(src_root, "limit", 20);
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

                memochat::json::JsonValue moments_arr(memochat::json::JsonValue{});
                for (const auto& moment : moments) {
                    bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                    MomentContentInfo content;
                    MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);
                    memochat::json::JsonValue moment_json;
                    BuildMomentJson(moment, has_liked, &content, moment_json);
                    memochat::json::glaze_append(moments_arr, moment_json);
                }
                root["moments"] = moments_arr;
                return true;
            });
    });

    logic.RegPost("/api/moments/detail", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "moment_id", 0LL);
                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                MomentInfo moment;
                if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                if (moment.uid != uid && moment.visibility == 2) {
                    root["error"] = ErrorCodes::CallPermissionDenied;
                    return true;
                }

                bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment_id, content);

                memochat::json::JsonValue moment_json;
                BuildMomentJson(moment, has_liked, &content, moment_json);
                root["error"] = ErrorCodes::Success;
                root["moment"] = moment_json;
                return true;
            });
    });

    logic.RegPost("/api/moments/delete", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "moment_id", 0LL);
                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                if (!PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid)) {
                    root["error"] = ErrorCodes::RPCFailed;
                    return true;
                }

                root["error"] = ErrorCodes::Success;
                return true;
            });
    });

    logic.RegPost("/api/moments/like", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "moment_id", 0LL);
                bool like = memochat::json::glaze_safe_get<bool>(src_root, "like", true);
                if (moment_id <= 0) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                bool ok = like
                    ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                    : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);

                root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                return true;
            });
    });

    logic.RegPost("/api/moments/comment", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "moment_id", 0LL);
                std::string content_str = memochat::json::glaze_safe_get<std::string>(src_root, "content", "");
                int reply_uid = memochat::json::glaze_safe_get<int>(src_root, "reply_uid", 0);
                int64_t comment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "comment_id", 0LL);
                bool delete_mode = memochat::json::glaze_safe_get<bool>(src_root, "delete", false);

                if (delete_mode) {
                    if (comment_id <= 0) {
                        root["error"] = ErrorCodes::Error_Json;
                        return true;
                    }
                    bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                    root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                    return true;
                }

                if (moment_id <= 0 || content_str.empty()) {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }

                MomentCommentInfo comment;
                comment.moment_id = moment_id;
                comment.uid = uid;
                comment.content = content_str;
                comment.reply_uid = reply_uid;
                comment.created_at = NowMs();

                bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
                root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                return true;
            });
    });

    logic.RegPost("/api/moments/comment/list", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                int uid = 0;
                if (!ValidateAuth(src_root, root, uid)) return true;

                int64_t moment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "moment_id", 0LL);
                int64_t last_comment_id = memochat::json::glaze_safe_get<int64_t>(src_root, "last_comment_id", 0LL);
                int limit = memochat::json::glaze_safe_get<int>(src_root, "limit", 20);
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

                memochat::json::JsonValue comments_arr(memochat::json::JsonValue{});
                for (const auto& comment : comments) {
                    memochat::json::JsonValue comment_json;
                    comment_json["id"] = static_cast<int64_t>(comment.id);
                    comment_json["moment_id"] = static_cast<int64_t>(comment.moment_id);
                    comment_json["uid"] = comment.uid;
                    comment_json["content"] = comment.content;
                    comment_json["reply_uid"] = comment.reply_uid;
                    comment_json["created_at"] = static_cast<int64_t>(comment.created_at);
                    BuildUserProfile(comment.uid, comment_json);
                    if (comment.reply_uid > 0) {
                        UserInfo reply_user;
                        if (PostgresMgr::GetInstance()->GetUserInfo(comment.reply_uid, reply_user)) {
                            comment_json["reply_nick"] = reply_user.nick;
                        }
                    }
                    memochat::json::glaze_append(comments_arr, comment_json);
                }
                root["comments"] = comments_arr;
                return true;
            });
    });
}

