#include "services/moments/MomentsService.h"

#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "clients/MomentsRelationClient.h"
#include "ConfigMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace memochat::gate::services::moments
{
namespace
{

namespace json = memochat::json;

// Relation client for cross-service friendship checks. The moments DB doesn't
// own the friend tables after the Phase 2 split, so visibility for friends-only
// posts is resolved over gRPC against the relation-query service. Endpoint comes
// from [RelationQueryService] Endpoint (default 127.0.0.1:50090).
MomentsRelationClient& RelationClient()
{
    static MomentsRelationClient client = []
    {
        std::string endpoint = ConfigMgr::Inst()["RelationQueryService"]["Endpoint"];
        if (endpoint.empty())
        {
            endpoint = "127.0.0.1:50090";
        }
        return MomentsRelationClient(endpoint);
    }();
    return client;
}

// Visibility check that doesn't touch the friend tables locally (they live in
// memo_pg, not the moments DB). Public + own are allowed directly; friends-only
// is resolved via the relation service. Mirrors the semantics of the retired
// PostgresDao::CanViewMoment.
bool CanViewMomentRemote(int viewer_uid, const MomentInfo& moment)
{
    if (viewer_uid <= 0 || moment.uid <= 0)
    {
        return false;
    }
    if (viewer_uid == moment.uid || moment.visibility == 0)
    {
        return true;
    }
    if (moment.visibility != 1)
    {
        return false;
    }
    const auto friend_uids = RelationClient().FilterFriendUids(viewer_uid, {moment.uid});
    return !friend_uids.empty();
}

void WriteJson(memochat::gate::routing::GateResponse& response, const json::JsonValue& root)
{
    response.status = 200;
    response.content_type = "application/json";
    response.body = json::glaze_stringify(root);
}

bool HandleJsonRequest(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response,
                       const std::function<bool(const json::JsonValue&, json::JsonValue&, const std::string&)>& fn)
{
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    fn(src_root, root, request.trace_id);
    root["trace_id"] = request.trace_id;
    WriteJson(response, root);
    return true;
}

bool ValidateAuth(const json::JsonValue& src_root, json::JsonValue& root, int& uid)
{
    if (!json::isMember(src_root, "uid") || !json::isMember(src_root, "login_ticket"))
    {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    if (uid <= 0)
    {
        root["error"] = ErrorCodes::UidInvalid;
        return false;
    }
    return true;
}

int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void NormalizeMomentItem(MomentItemInfo& item)
{
    if (item.media_type != "image" && item.media_type != "video")
    {
        item.media_type = "text";
    }
    if ((item.media_type == "image" || item.media_type == "video") && item.media_key.empty())
    {
        item.media_type = "text";
    }
    if (item.media_type == "text")
    {
        item.media_key.clear();
        item.thumb_key.clear();
        item.width = 0;
        item.height = 0;
        item.duration_ms = 0;
    }
}

json::JsonValue ReadMember(const json::JsonValue& obj, const char* key)
{
    if (!json::isMember(obj, key))
    {
        return json::JsonValue();
    }
    return obj[key];
}

int64_t ReadInt64(const json::JsonValue& obj, const char* key, int64_t fallback = 0)
{
    const json::JsonValue value = ReadMember(obj, key);
    try
    {
        return value.asInt64();
    }
    catch (...)
    {
    }
    try
    {
        return static_cast<int64_t>(value.asInt());
    }
    catch (...)
    {
    }
    try
    {
        return static_cast<int64_t>(value.asDouble());
    }
    catch (...)
    {
    }
    const std::string text = value.asString();
    if (!text.empty())
    {
        char* end = nullptr;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        if (end && *end == '\0')
        {
            return static_cast<int64_t>(parsed);
        }
    }
    return fallback;
}

int ReadInt(const json::JsonValue& obj, const char* key, int fallback = 0)
{
    return static_cast<int>(ReadInt64(obj, key, fallback));
}

bool ReadBool(const json::JsonValue& obj, const char* key, bool fallback = false)
{
    const json::JsonValue value = ReadMember(obj, key);
    try
    {
        return value.asBool();
    }
    catch (...)
    {
    }
    const std::string text = value.asString();
    if (text == "true" || text == "1")
    {
        return true;
    }
    if (text == "false" || text == "0")
    {
        return false;
    }
    try
    {
        return value.asInt() != 0;
    }
    catch (...)
    {
    }
    try
    {
        return value.asDouble() != 0.0;
    }
    catch (...)
    {
    }
    return fallback;
}

std::string ReadString(const json::JsonValue& obj, const char* key, const std::string& fallback = {})
{
    const json::JsonValue value = ReadMember(obj, key);
    const std::string text = value.asString();
    return text.empty() ? fallback : text;
}

void BuildMomentItem(const MomentItemInfo& item, json::JsonValue& json)
{
    json["seq"] = item.seq;
    json["media_type"] = item.media_type;
    json["media_key"] = item.media_key;
    json["thumb_key"] = item.thumb_key;
    json["content"] = item.content;
    json["width"] = item.width;
    json["height"] = item.height;
    json["duration_ms"] = item.duration_ms;
}

void BuildUserProfile(int uid, json::JsonValue& json)
{
    UserInfo user_info;
    if (PostgresMgr::GetInstance()->GetUserInfo(uid, user_info))
    {
        json["user_id"] = user_info.user_id;
        json["user_name"] = user_info.name;
        json["user_nick"] = user_info.nick;
        json["user_icon"] = user_info.icon;
    }
    else
    {
        json["user_id"] = "";
        json["user_name"] = "";
        json["user_nick"] = "";
        json["user_icon"] = "";
    }
}

void BuildLikeList(const std::vector<MomentLikeInfo>& likes, json::JsonValue& json)
{
    json = json::glaze_make_array();
    for (const auto& like : likes)
    {
        json::JsonValue item;
        item["uid"] = like.uid;
        item["user_nick"] = like.user_nick;
        item["user_icon"] = like.user_icon;
        item["created_at"] = static_cast<int64_t>(like.created_at);
        json::glaze_append(json, item);
    }
}

void BuildCommentList(const std::vector<MomentCommentInfo>& comments, json::JsonValue& json)
{
    json = json::glaze_make_array();
    for (const auto& comment : comments)
    {
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
        for (const auto& like : comment.likes)
        {
            if (!like.user_nick.empty())
            {
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

void FillCommentLikes(int viewer_uid, std::vector<MomentCommentInfo>& comments)
{
    for (auto& comment : comments)
    {
        bool has_more = false;
        std::vector<MomentLikeInfo> likes;
        if (PostgresMgr::GetInstance()->GetMomentCommentLikes(comment.id, 100, likes, has_more))
        {
            comment.likes = std::move(likes);
            comment.like_count = static_cast<int>(comment.likes.size());
        }
        comment.has_liked = PostgresMgr::GetInstance()->HasLikedMomentComment(comment.id, viewer_uid);
    }
}

void BuildMomentJson(const MomentInfo& moment,
                     bool has_liked,
                     const MomentContentInfo* content,
                     const std::vector<MomentLikeInfo>* likes,
                     const std::vector<MomentCommentInfo>* comments,
                     json::JsonValue& json)
{
    json["moment_id"] = static_cast<int64_t>(moment.moment_id);
    json["uid"] = moment.uid;
    json["visibility"] = moment.visibility;
    json["location"] = moment.location;
    json["created_at"] = static_cast<int64_t>(moment.created_at);
    json["like_count"] = moment.like_count;
    json["comment_count"] = moment.comment_count;
    json["has_liked"] = has_liked;
    BuildUserProfile(moment.uid, json);

    if (content)
    {
        json::JsonValue items_json = json::glaze_make_array();
        for (const auto& item : content->items)
        {
            json::JsonValue item_json;
            BuildMomentItem(item, item_json);
            json::glaze_append(items_json, item_json);
        }
        json["items"] = items_json;
    }
    else
    {
        json["items"] = json::glaze_make_array();
    }

    if (likes)
    {
        json::JsonValue like_names = json::glaze_make_array();
        for (const auto& like : *likes)
        {
            if (!like.user_nick.empty())
            {
                json::glaze_append(like_names, like.user_nick);
            }
        }
        json["like_names"] = like_names;
        json::JsonValue likes_arr = json::glaze_make_array();
        BuildLikeList(*likes, likes_arr);
        json["likes"] = likes_arr;
    }
    else
    {
        json["like_names"] = json::glaze_make_array();
        json["likes"] = json::glaze_make_array();
    }

    if (comments)
    {
        json::JsonValue comments_arr = json::glaze_make_array();
        BuildCommentList(*comments, comments_arr);
        json["comments"] = comments_arr;
    }
    else
    {
        json["comments"] = json::glaze_make_array();
    }
}

} // namespace

MomentsService& MomentsService::Instance()
{
    static MomentsService instance;
    return instance;
}

bool MomentsService::HandlePublish(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 if (!ValidateAuth(src_root, root, uid))
                                 {
                                     return true;
                                 }

                                 const int visibility = ReadInt(src_root, "visibility", 0);
                                 const std::string location = ReadString(src_root, "location");
                                 const auto now_ms = NowMs();

                                 MomentInfo moment;
                                 moment.uid = uid;
                                 moment.visibility = visibility;
                                 moment.location = location;
                                 moment.created_at = now_ms;
                                 moment.like_count = 0;
                                 moment.comment_count = 0;

                                 int64_t moment_id = 0;
                                 if (!PostgresMgr::GetInstance()->AddMoment(moment, &moment_id))
                                 {
                                     root["error"] = ErrorCodes::RPCFailed;
                                     return true;
                                 }

                                 MomentContentInfo content;
                                 content.moment_id = moment_id;
                                 content.server_time = now_ms;

                                 if (json::isMember(src_root, "items") && src_root["items"].isArray())
                                 {
                                     for (const auto& item_json : src_root["items"])
                                     {
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
                                 if (content.items.empty() && !raw_content.empty())
                                 {
                                     MomentItemInfo item;
                                     item.media_type = "text";
                                     item.content = raw_content;
                                     NormalizeMomentItem(item);
                                     content.items.push_back(item);
                                 }

                                 if (moment_id > 0 && !MongoMgr::GetInstance()->InsertMomentContent(content))
                                 {
                                     PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid);
                                     root["error"] = ErrorCodes::RPCFailed;
                                     memolog::LogError(
                                         "gate.moments.publish",
                                         "failed to store moment content in MongoDB",
                                         {{"moment_id", std::to_string(moment_id)}, {"uid", std::to_string(uid)}});
                                     return true;
                                 }

                                 root["error"] = ErrorCodes::Success;
                                 root["moment_id"] = static_cast<int64_t>(moment_id);
                                 memolog::LogInfo("gate.moments.publish.ok",
                                                  "moment published with content",
                                                  {{"moment_id", std::to_string(moment_id)},
                                                   {"uid", std::to_string(uid)},
                                                   {"items", std::to_string(content.items.size())}});
                                 return true;
                             });
}

bool MomentsService::HandleList(const memochat::gate::routing::GateRequest& request,
                                memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
        {
            int uid = 0;
            if (!ValidateAuth(src_root, root, uid))
            {
                return true;
            }

            const int64_t last_moment_id = ReadInt64(src_root, "last_moment_id", 0);
            int limit = ReadInt(src_root, "limit", 20);
            const int author_uid = ReadInt(src_root, "author_uid", 0);
            if (limit <= 0)
            {
                limit = 20;
            }
            if (limit > 50)
            {
                limit = 50;
            }

            std::vector<MomentInfo> moments;
            bool has_more = false;
            // After the Phase 2 DB split the moments database no longer owns the
            // friend tables, so we fetch candidates (public + own + ALL
            // friends-only) without the friend subquery, then drop friends-only
            // posts whose author isn't a friend of the viewer using the relation
            // service. Public and own posts are always kept.
            if (!PostgresMgr::GetInstance()->GetMomentsFeedCandidates(
                    uid, last_moment_id, limit, author_uid, moments, has_more))
            {
                root["error"] = ErrorCodes::RPCFailed;
                return true;
            }

            std::vector<int> friends_only_authors;
            for (const auto& moment : moments)
            {
                if (moment.visibility == 1 && moment.uid != uid)
                {
                    friends_only_authors.push_back(moment.uid);
                }
            }
            std::unordered_set<int> visible_friend_authors;
            if (!friends_only_authors.empty())
            {
                const auto friend_uids = RelationClient().FilterFriendUids(uid, friends_only_authors);
                visible_friend_authors.insert(friend_uids.begin(), friend_uids.end());
            }

            root["error"] = ErrorCodes::Success;
            root["has_more"] = has_more;

            json::JsonValue moments_arr = json::glaze_make_array();
            for (const auto& moment : moments)
            {
                // Drop friends-only posts from non-friends (own/public always kept).
                if (moment.visibility == 1 && moment.uid != uid &&
                    visible_friend_authors.find(moment.uid) == visible_friend_authors.end())
                {
                    continue;
                }
                const bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment.moment_id, uid);
                MomentContentInfo content;
                MongoMgr::GetInstance()->GetMomentContent(moment.moment_id, content);

                json::JsonValue moment_json;
                BuildMomentJson(moment, has_liked, &content, nullptr, nullptr, moment_json);
                json::glaze_append(moments_arr, moment_json);
            }
            root["moments"] = moments_arr;
            return true;
        });
}

bool MomentsService::HandleDetail(const memochat::gate::routing::GateRequest& request,
                                  memochat::gate::routing::GateResponse& response)
{
    memolog::LogInfo("gate.moments.detail", "detail handler entered");
    return HandleJsonRequest(
        request,
        response,
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
        {
            int uid = 0;
            if (!ValidateAuth(src_root, root, uid))
            {
                memolog::LogWarn("gate.moments.detail", "auth failed");
                return true;
            }

            const int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
            memolog::LogInfo("gate.moments.detail", "moment_id=" + std::to_string(moment_id));
            if (moment_id <= 0)
            {
                root["error"] = ErrorCodes::Error_Json;
                memolog::LogWarn("gate.moments.detail", "invalid moment_id");
                return true;
            }

            MomentInfo moment;
            if (!PostgresMgr::GetInstance()->GetMomentById(moment_id, moment))
            {
                root["error"] = ErrorCodes::RPCFailed;
                memolog::LogWarn("gate.moments.detail", "GetMomentById failed, moment_id=" + std::to_string(moment_id));
                return true;
            }

            if (!CanViewMomentRemote(uid, moment))
            {
                root["error"] = ErrorCodes::CallPermissionDenied;
                return true;
            }

            const bool has_liked = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
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

            json::JsonValue moment_json;
            BuildMomentJson(moment, has_liked, &content, &likes, &comments, moment_json);
            root["error"] = ErrorCodes::Success;
            root["moment"] = moment_json;
            return true;
        });
}

bool MomentsService::HandleDelete(const memochat::gate::routing::GateRequest& request,
                                  memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 if (!ValidateAuth(src_root, root, uid))
                                 {
                                     return true;
                                 }

                                 const int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
                                 if (moment_id <= 0)
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 if (!PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid))
                                 {
                                     root["error"] = ErrorCodes::RPCFailed;
                                     return true;
                                 }

                                 root["error"] = ErrorCodes::Success;
                                 root["moment_id"] = static_cast<int64_t>(moment_id);
                                 return true;
                             });
}

bool MomentsService::HandleLike(const memochat::gate::routing::GateRequest& request,
                                memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 if (!ValidateAuth(src_root, root, uid))
                                 {
                                     return true;
                                 }

                                 const int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
                                 const bool like = ReadBool(src_root, "like", true);
                                 if (moment_id <= 0)
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 const bool ok = like ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                                                      : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);

                                 if (!ok)
                                 {
                                     root["error"] = ErrorCodes::RPCFailed;
                                     return true;
                                 }

                                 MomentInfo moment;
                                 PostgresMgr::GetInstance()->GetMomentById(moment_id, moment);
                                 root["error"] = ErrorCodes::Success;
                                 root["moment_id"] = static_cast<int64_t>(moment_id);
                                 root["has_liked"] = PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
                                 root["like_count"] = moment.like_count;
                                 return true;
                             });
}

bool MomentsService::HandleComment(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 if (!ValidateAuth(src_root, root, uid))
                                 {
                                     return true;
                                 }

                                 const int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
                                 const std::string content = ReadString(src_root, "content");
                                 const int reply_uid = ReadInt(src_root, "reply_uid", 0);
                                 const int64_t comment_id = ReadInt64(src_root, "comment_id", 0);
                                 const bool delete_mode =
                                     ReadBool(src_root, "delete", false) || (comment_id > 0 && content.empty());

                                 if (delete_mode)
                                 {
                                     if (comment_id <= 0)
                                     {
                                         root["error"] = ErrorCodes::Error_Json;
                                         return true;
                                     }
                                     const bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
                                     root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                                     root["moment_id"] = static_cast<int64_t>(moment_id);
                                     root["delete"] = true;
                                     MomentInfo moment;
                                     if (moment_id > 0 && PostgresMgr::GetInstance()->GetMomentById(moment_id, moment))
                                     {
                                         root["comment_count"] = moment.comment_count;
                                     }
                                     return true;
                                 }

                                 if (moment_id <= 0 || content.empty())
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 MomentCommentInfo comment;
                                 comment.moment_id = moment_id;
                                 comment.uid = uid;
                                 comment.content = content;
                                 comment.reply_uid = reply_uid;
                                 comment.created_at = NowMs();

                                 const bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
                                 root["error"] = ok ? ErrorCodes::Success : ErrorCodes::RPCFailed;
                                 root["moment_id"] = static_cast<int64_t>(moment_id);
                                 root["delete"] = false;
                                 MomentInfo moment;
                                 if (PostgresMgr::GetInstance()->GetMomentById(moment_id, moment))
                                 {
                                     root["comment_count"] = moment.comment_count;
                                 }
                                 return true;
                             });
}

bool MomentsService::HandleCommentList(const memochat::gate::routing::GateRequest& request,
                                       memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
        {
            int uid = 0;
            if (!ValidateAuth(src_root, root, uid))
            {
                return true;
            }

            const int64_t moment_id = ReadInt64(src_root, "moment_id", 0);
            const int64_t last_comment_id = ReadInt64(src_root, "last_comment_id", 0);
            int limit = ReadInt(src_root, "limit", 20);
            if (limit <= 0)
            {
                limit = 20;
            }
            if (limit > 50)
            {
                limit = 50;
            }

            if (moment_id <= 0)
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            std::vector<MomentCommentInfo> comments;
            bool has_more = false;
            if (!PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more))
            {
                root["error"] = ErrorCodes::RPCFailed;
                return true;
            }
            FillCommentLikes(uid, comments);

            root["error"] = ErrorCodes::Success;
            root["moment_id"] = static_cast<int64_t>(moment_id);
            root["has_more"] = has_more;
            MomentInfo moment;
            if (PostgresMgr::GetInstance()->GetMomentById(moment_id, moment))
            {
                root["comment_count"] = moment.comment_count;
            }

            json::JsonValue comments_arr = json::glaze_make_array();
            BuildCommentList(comments, comments_arr);
            root["comments"] = comments_arr;
            return true;
        });
}

bool MomentsService::HandleCommentLike(const memochat::gate::routing::GateRequest& request,
                                       memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 if (!ValidateAuth(src_root, root, uid))
                                 {
                                     return true;
                                 }

                                 const int64_t comment_id = ReadInt64(src_root, "comment_id", 0);
                                 const bool like = ReadBool(src_root, "like", true);
                                 if (comment_id <= 0)
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 const bool ok =
                                     like ? PostgresMgr::GetInstance()->AddMomentCommentLike(comment_id, uid)
                                          : PostgresMgr::GetInstance()->RemoveMomentCommentLike(comment_id, uid);
                                 if (!ok)
                                 {
                                     root["error"] = ErrorCodes::RPCFailed;
                                     return true;
                                 }

                                 std::vector<MomentLikeInfo> likes;
                                 bool has_more = false;
                                 PostgresMgr::GetInstance()->GetMomentCommentLikes(comment_id, 100, likes, has_more);

                                 root["error"] = ErrorCodes::Success;
                                 root["comment_id"] = static_cast<int64_t>(comment_id);
                                 root["has_liked"] = PostgresMgr::GetInstance()->HasLikedMomentComment(comment_id, uid);
                                 root["like_count"] = static_cast<int>(likes.size());
                                 json::JsonValue like_names = json::glaze_make_array();
                                 for (const auto& item : likes)
                                 {
                                     if (!item.user_nick.empty())
                                     {
                                         json::glaze_append(like_names, item.user_nick);
                                     }
                                 }
                                 root["like_names"] = like_names;
                                 json::JsonValue likes_arr = json::glaze_make_array();
                                 for (const auto& item : likes)
                                 {
                                     json::JsonValue like_json;
                                     like_json["uid"] = item.uid;
                                     like_json["user_nick"] = item.user_nick;
                                     like_json["user_icon"] = item.user_icon;
                                     like_json["created_at"] = static_cast<int64_t>(item.created_at);
                                     json::glaze_append(likes_arr, like_json);
                                 }
                                 root["likes"] = likes_arr;
                                 return true;
                             });
}

} // namespace memochat::gate::services::moments
