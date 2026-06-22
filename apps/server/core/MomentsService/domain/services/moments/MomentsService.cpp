#include "services/moments/MomentsService.h"

#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "clients/MomentsRelationClient.h"
#include "ConfigMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"
#include "services/moments/MomentsOutputDtos.h"
#include "services/moments/MomentsPublicDtos.h"

#include <chrono>
#include <functional>
#include <optional>
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

MomentUserProfileDto BuildUserProfile(int uid)
{
    MomentUserProfileDto profile;
    UserInfo user_info;
    if (PostgresMgr::GetInstance()->GetUserInfo(uid, user_info))
    {
        profile.user_id = user_info.user_id;
        profile.user_name = user_info.name;
        profile.user_nick = user_info.nick;
        profile.user_icon = user_info.icon;
    }
    return profile;
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

} // namespace

MomentsService& MomentsService::Instance()
{
    static MomentsService instance;
    return instance;
}

bool MomentsService::HandlePublish(const memochat::gate::routing::GateRequest& request,
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

            const MomentPublishRequestDto request_dto = MomentPublishRequestFromJsonValue(src_root);
            const auto now_ms = NowMs();

            MomentInfo moment;
            moment.uid = uid;
            moment.visibility = request_dto.visibility;
            moment.location = request_dto.location;
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
            content.items = request_dto.items;

            if (moment_id > 0 && !MongoMgr::GetInstance()->InsertMomentContent(content))
            {
                PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid);
                root["error"] = ErrorCodes::RPCFailed;
                memolog::LogError("gate.moments.publish",
                                  "failed to store moment content in MongoDB",
                                  {{"moment_id", std::to_string(moment_id)}, {"uid", std::to_string(uid)}});
                return true;
            }

            root = ToJsonValue(MomentIdResponseDto{ErrorCodes::Success, moment_id});
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

            const MomentListRequestDto request_dto = MomentListRequestFromJsonValue(src_root);

            std::vector<MomentInfo> moments;
            bool has_more = false;
            // After the Phase 2 DB split the moments database no longer owns the
            // friend tables, so we fetch candidates (public + own + ALL
            // friends-only) without the friend subquery, then drop friends-only
            // posts whose author isn't a friend of the viewer using the relation
            // service. Public and own posts are always kept.
            if (!PostgresMgr::GetInstance()->GetMomentsFeedCandidates(uid,
                                                                      request_dto.last_moment_id,
                                                                      request_dto.limit,
                                                                      request_dto.author_uid,
                                                                      moments,
                                                                      has_more))
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

            std::vector<MomentOutputDto> moment_dtos;
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

                moment_dtos.push_back(
                    ToMomentOutputDto(moment, has_liked, BuildUserProfile(moment.uid), &content, nullptr, nullptr));
            }
            root = ToJsonValue(MomentListResponseDto{ErrorCodes::Success, has_more, std::move(moment_dtos)});
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

            const MomentIdRequestDto request_dto = MomentIdRequestFromJsonValue(src_root);
            const int64_t moment_id = request_dto.moment_id;
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

            root = ToJsonValue(MomentDetailResponseDto{
                ErrorCodes::Success,
                ToMomentOutputDto(moment, has_liked, BuildUserProfile(moment.uid), &content, &likes, &comments)});
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

                                 const MomentIdRequestDto request_dto = MomentIdRequestFromJsonValue(src_root);
                                 const int64_t moment_id = request_dto.moment_id;
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

                                 root = ToJsonValue(MomentIdResponseDto{ErrorCodes::Success, moment_id});
                                 return true;
                             });
}

bool MomentsService::HandleLike(const memochat::gate::routing::GateRequest& request,
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

            const MomentLikeRequestDto request_dto = MomentLikeRequestFromJsonValue(src_root);
            if (request_dto.moment_id <= 0)
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            const bool ok = request_dto.like ? PostgresMgr::GetInstance()->AddMomentLike(request_dto.moment_id, uid)
                                             : PostgresMgr::GetInstance()->RemoveMomentLike(request_dto.moment_id, uid);

            if (!ok)
            {
                root["error"] = ErrorCodes::RPCFailed;
                return true;
            }

            MomentInfo moment;
            PostgresMgr::GetInstance()->GetMomentById(request_dto.moment_id, moment);
            root = ToJsonValue(
                MomentLikeResponseDto{ErrorCodes::Success,
                                      request_dto.moment_id,
                                      PostgresMgr::GetInstance()->HasLikedMoment(request_dto.moment_id, uid),
                                      moment.like_count});
            return true;
        });
}

bool MomentsService::HandleComment(const memochat::gate::routing::GateRequest& request,
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

            const MomentCommentRequestDto request_dto = MomentCommentRequestFromJsonValue(src_root);

            if (request_dto.delete_mode)
            {
                if (request_dto.comment_id <= 0)
                {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }
                const bool ok = PostgresMgr::GetInstance()->DeleteMomentComment(request_dto.comment_id, uid);
                std::optional<int> comment_count;
                MomentInfo moment;
                if (request_dto.moment_id > 0 &&
                    PostgresMgr::GetInstance()->GetMomentById(request_dto.moment_id, moment))
                {
                    comment_count = moment.comment_count;
                }
                if (ok)
                {
                    root = ToJsonValue(MomentCommentMutationResponseDto{ErrorCodes::Success,
                                                                        request_dto.moment_id,
                                                                        true,
                                                                        comment_count});
                    return true;
                }
                root = ToJsonValue(MomentCommentMutationResponseDto{ErrorCodes::RPCFailed,
                                                                    request_dto.moment_id,
                                                                    true,
                                                                    comment_count});
                return true;
            }

            if (request_dto.moment_id <= 0 || request_dto.content.empty())
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            MomentCommentInfo comment;
            comment.moment_id = request_dto.moment_id;
            comment.uid = uid;
            comment.content = request_dto.content;
            comment.reply_uid = request_dto.reply_uid;
            comment.created_at = NowMs();

            const bool ok = PostgresMgr::GetInstance()->AddMomentComment(comment);
            std::optional<int> comment_count;
            MomentInfo moment;
            if (PostgresMgr::GetInstance()->GetMomentById(request_dto.moment_id, moment))
            {
                comment_count = moment.comment_count;
            }
            if (ok)
            {
                root = ToJsonValue(
                    MomentCommentMutationResponseDto{ErrorCodes::Success, request_dto.moment_id, false, comment_count});
                return true;
            }
            root = ToJsonValue(
                MomentCommentMutationResponseDto{ErrorCodes::RPCFailed, request_dto.moment_id, false, comment_count});
            return true;
        });
}

bool MomentsService::HandleCommentList(const memochat::gate::routing::GateRequest& request,
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

                                 const MomentCommentListRequestDto request_dto =
                                     MomentCommentListRequestFromJsonValue(src_root);

                                 if (request_dto.moment_id <= 0)
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 std::vector<MomentCommentInfo> comments;
                                 bool has_more = false;
                                 if (!PostgresMgr::GetInstance()->GetMomentComments(request_dto.moment_id,
                                                                                    request_dto.last_comment_id,
                                                                                    request_dto.limit,
                                                                                    comments,
                                                                                    has_more))
                                 {
                                     root["error"] = ErrorCodes::RPCFailed;
                                     return true;
                                 }
                                 FillCommentLikes(uid, comments);

                                 std::optional<int> comment_count;
                                 MomentInfo moment;
                                 if (PostgresMgr::GetInstance()->GetMomentById(request_dto.moment_id, moment))
                                 {
                                     comment_count = moment.comment_count;
                                 }

                                 root = ToJsonValue(MomentCommentListResponseDto{ErrorCodes::Success,
                                                                                 request_dto.moment_id,
                                                                                 has_more,
                                                                                 comment_count,
                                                                                 ToMomentCommentOutputDtos(comments)});
                                 return true;
                             });
}

bool MomentsService::HandleCommentLike(const memochat::gate::routing::GateRequest& request,
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

            const MomentCommentLikeRequestDto request_dto = MomentCommentLikeRequestFromJsonValue(src_root);
            if (request_dto.comment_id <= 0)
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            const bool ok = request_dto.like
                                ? PostgresMgr::GetInstance()->AddMomentCommentLike(request_dto.comment_id, uid)
                                : PostgresMgr::GetInstance()->RemoveMomentCommentLike(request_dto.comment_id, uid);
            if (!ok)
            {
                root["error"] = ErrorCodes::RPCFailed;
                return true;
            }

            std::vector<MomentLikeInfo> likes;
            bool has_more = false;
            PostgresMgr::GetInstance()->GetMomentCommentLikes(request_dto.comment_id, 100, likes, has_more);

            root = ToJsonValue(MomentCommentLikeResponseDto{
                ErrorCodes::Success,
                request_dto.comment_id,
                PostgresMgr::GetInstance()->HasLikedMomentComment(request_dto.comment_id, uid),
                static_cast<int>(likes.size()),
                ToMomentLikeNames(likes),
                ToMomentLikeOutputDtos(likes)});
            return true;
        });
}

} // namespace memochat::gate::services::moments
