#include "services/moments/MomentsService.hpp"

#include "MongoMgr.hpp"
#include "clients/MomentsRelationClient.hpp"
#include "ConfigMgr.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "logging/Logger.hpp"
#include "services/moments/MomentsOutputDtos.hpp"
#include "services/moments/MomentsPersistence.hpp"
#include "services/moments/MomentsPublicDtos.hpp"
#include "support/BearerAccessAuth.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

import memochat.moments.service_algorithms;

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
        if (memochat::moments::service::modules::ShouldUseDefaultRelationEndpoint(endpoint.empty()))
        {
            endpoint = memochat::moments::service::modules::DefaultRelationQueryEndpoint();
        }
        return MomentsRelationClient(endpoint);
    }();
    return client;
}

// Composition-local Meyers singleton — GetInstance() does not belong here;
// the concrete adapter owns all PostgresMgr interaction.
IMomentsPersistence& Persistence()
{
    static MomentsPersistence instance;
    return instance;
}

// Visibility check that doesn't touch the friend tables locally (they live in
// memo_pg, not the moments DB). Public + own are allowed directly; friends-only
// is resolved via the relation service. Mirrors the semantics of the retired
// local DAO visibility check.
bool CanViewMomentRemote(int viewer_uid, const MomentInfo& moment)
{
    if (!memochat::moments::service::modules::HasValidViewerAndAuthor(viewer_uid, moment.uid))
    {
        return false;
    }
    if (memochat::moments::service::modules::CanViewWithoutRelationCheck(viewer_uid, moment.uid, moment.visibility))
    {
        return true;
    }
    if (memochat::moments::service::modules::ShouldRejectNonFriendsVisibility(moment.visibility))
    {
        return false;
    }
    const auto friend_uids = RelationClient().FilterFriendUids(viewer_uid, {moment.uid});
    return !friend_uids.empty();
}

void WriteJson(memochat::gate::routing::GateResponse& response, const json::JsonValue& root)
{
    response.status = memochat::moments::service::modules::SuccessHttpStatus();
    response.content_type = memochat::moments::service::modules::JsonContentType();
    response.body = json::glaze_stringify(root);
}

bool HandleJsonRequest(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response,
                       const std::function<bool(const json::JsonValue&, json::JsonValue&, const std::string&, int)>& fn)
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

    int uid = 0;
    if (!memochat::auth::ResolveBearerAccessUserId(request, uid))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        WriteJson(response, root);
        return true;
    }

    fn(src_root, root, request.trace_id, uid);
    root["trace_id"] = request.trace_id;
    WriteJson(response, root);
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
    MomentUserProfileRow user_info;
    if (Persistence().LoadUserProfile(uid, user_info))
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
        if (Persistence().LoadCommentLikes(comment.id,
                                           memochat::moments::service::modules::CommentLikeFetchLimit(),
                                           likes,
                                           has_more))
        {
            comment.likes = std::move(likes);
            comment.like_count = static_cast<int>(comment.likes.size());
        }
        comment.has_liked = Persistence().HasCommentLike(comment.id, viewer_uid);
    }
}

bool LoadVisibleMoment(int viewer_uid, int64_t moment_id, MomentInfo& moment, json::JsonValue& root)
{
    if (!Persistence().LoadMoment(moment_id, moment))
    {
        root["error"] = ErrorCodes::RPCFailed;
        return false;
    }
    if (!CanViewMomentRemote(viewer_uid, moment))
    {
        root["error"] = ErrorCodes::CallPermissionDenied;
        return false;
    }
    return true;
}

bool LoadVisibleMomentForComment(int viewer_uid,
                                 int64_t comment_id,
                                 MomentCommentInfo& comment,
                                 MomentInfo& moment,
                                 json::JsonValue& root)
{
    if (!Persistence().LoadComment(comment_id, comment))
    {
        root["error"] = ErrorCodes::RPCFailed;
        return false;
    }
    return LoadVisibleMoment(viewer_uid, comment.moment_id, moment, root);
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
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
        {
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
            if (!Persistence().CreateMoment(moment, &moment_id))
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
                Persistence().DeleteMoment(moment_id, uid);
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
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
        {
            const MomentListRequestDto request_dto = MomentListRequestFromJsonValue(src_root);

            std::vector<MomentInfo> moments;
            bool has_more = false;
            // After the Phase 2 DB split the moments database no longer owns the
            // friend tables, so we fetch candidates (public + own + ALL
            // friends-only) without the friend subquery, then drop friends-only
            // posts whose author isn't a friend of the viewer using the relation
            // service. Public and own posts are always kept.
            if (!Persistence().LoadFeedCandidates(uid,
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
                if (memochat::moments::service::modules::IsFriendsOnlyForeignMoment(moment.visibility, uid, moment.uid))
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
                if (memochat::moments::service::modules::IsFriendsOnlyForeignMoment(moment.visibility,
                                                                                    uid,
                                                                                    moment.uid) &&
                    visible_friend_authors.find(moment.uid) == visible_friend_authors.end())
                {
                    continue;
                }
                const bool has_liked = Persistence().HasMomentLike(moment.moment_id, uid);
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
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
        {
            const MomentIdRequestDto request_dto = MomentIdRequestFromJsonValue(src_root);
            const int64_t moment_id = request_dto.moment_id;
            memolog::LogInfo("gate.moments.detail", "moment_id=" + std::to_string(moment_id));
            if (!memochat::moments::service::modules::HasValidMomentId(moment_id))
            {
                root["error"] = ErrorCodes::Error_Json;
                memolog::LogWarn("gate.moments.detail", "invalid moment_id");
                return true;
            }

            MomentInfo moment;
            if (!Persistence().LoadMoment(moment_id, moment))
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

            const bool has_liked = Persistence().HasMomentLike(moment_id, uid);
            MomentContentInfo content;
            MongoMgr::GetInstance()->GetMomentContent(moment_id, content);

            std::vector<MomentLikeInfo> likes;
            bool likes_has_more = false;
            Persistence().LoadMomentLikes(moment_id,
                                          memochat::moments::service::modules::DetailLikeFetchLimit(),
                                          likes,
                                          likes_has_more);

            std::vector<MomentCommentInfo> comments;
            bool comments_has_more = false;
            Persistence().LoadComments(moment_id,
                                       0,
                                       memochat::moments::service::modules::DetailCommentFetchLimit(),
                                       comments,
                                       comments_has_more);
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
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
                             {
                                 const MomentIdRequestDto request_dto = MomentIdRequestFromJsonValue(src_root);
                                 const int64_t moment_id = request_dto.moment_id;
                                 if (!memochat::moments::service::modules::HasValidMomentId(moment_id))
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 if (!Persistence().DeleteMoment(moment_id, uid))
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
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
        {
            const MomentLikeRequestDto request_dto = MomentLikeRequestFromJsonValue(src_root);
            if (!memochat::moments::service::modules::HasValidMomentId(request_dto.moment_id))
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            MomentInfo moment;
            if (!LoadVisibleMoment(uid, request_dto.moment_id, moment, root))
            {
                return true;
            }

            const bool ok = Persistence().SetMomentLike(request_dto.moment_id, uid, request_dto.like);

            if (!ok)
            {
                root["error"] = ErrorCodes::RPCFailed;
                return true;
            }

            Persistence().LoadMoment(request_dto.moment_id, moment);
            root = ToJsonValue(MomentLikeResponseDto{ErrorCodes::Success,
                                                     request_dto.moment_id,
                                                     Persistence().HasMomentLike(request_dto.moment_id, uid),
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
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
        {
            const MomentCommentRequestDto request_dto = MomentCommentRequestFromJsonValue(src_root);

            if (request_dto.delete_mode)
            {
                if (!memochat::moments::service::modules::HasValidCommentId(request_dto.comment_id))
                {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }
                MomentCommentInfo deleted_comment;
                MomentInfo moment;
                if (!LoadVisibleMomentForComment(uid, request_dto.comment_id, deleted_comment, moment, root))
                {
                    return true;
                }
                if (request_dto.moment_id > 0 && request_dto.moment_id != deleted_comment.moment_id)
                {
                    root["error"] = ErrorCodes::Error_Json;
                    return true;
                }
                const bool ok = Persistence().DeleteComment(request_dto.comment_id, uid);
                std::optional<int> comment_count;
                if (Persistence().LoadMoment(deleted_comment.moment_id, moment))
                {
                    comment_count = moment.comment_count;
                }
                if (ok)
                {
                    root = ToJsonValue(MomentCommentMutationResponseDto{ErrorCodes::Success,
                                                                        deleted_comment.moment_id,
                                                                        true,
                                                                        comment_count});
                    return true;
                }
                root = ToJsonValue(MomentCommentMutationResponseDto{ErrorCodes::RPCFailed,
                                                                    deleted_comment.moment_id,
                                                                    true,
                                                                    comment_count});
                return true;
            }

            if (!memochat::moments::service::modules::HasValidNewComment(request_dto.moment_id,
                                                                         request_dto.content.empty()))
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            MomentInfo moment;
            if (!LoadVisibleMoment(uid, request_dto.moment_id, moment, root))
            {
                return true;
            }

            MomentCommentInfo comment;
            comment.moment_id = request_dto.moment_id;
            comment.uid = uid;
            comment.content = request_dto.content;
            comment.reply_uid = request_dto.reply_uid;
            comment.created_at = NowMs();

            const bool ok = Persistence().AddComment(comment);
            std::optional<int> comment_count;
            if (Persistence().LoadMoment(request_dto.moment_id, moment))
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
                             [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
                             {
                                 const MomentCommentListRequestDto request_dto =
                                     MomentCommentListRequestFromJsonValue(src_root);

                                 if (!memochat::moments::service::modules::HasValidMomentId(request_dto.moment_id))
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     return true;
                                 }

                                 MomentInfo moment;
                                 if (!LoadVisibleMoment(uid, request_dto.moment_id, moment, root))
                                 {
                                     return true;
                                 }

                                 std::vector<MomentCommentInfo> comments;
                                 bool has_more = false;
                                 if (!Persistence().LoadComments(request_dto.moment_id,
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
                                 if (Persistence().LoadMoment(request_dto.moment_id, moment))
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
        [](const json::JsonValue& src_root, json::JsonValue& root, const std::string&, int uid)
        {
            const MomentCommentLikeRequestDto request_dto = MomentCommentLikeRequestFromJsonValue(src_root);
            if (!memochat::moments::service::modules::HasValidCommentId(request_dto.comment_id))
            {
                root["error"] = ErrorCodes::Error_Json;
                return true;
            }

            MomentCommentInfo comment;
            MomentInfo moment;
            if (!LoadVisibleMomentForComment(uid, request_dto.comment_id, comment, moment, root))
            {
                return true;
            }

            const bool ok = Persistence().SetCommentLike(request_dto.comment_id, uid, request_dto.like);
            if (!ok)
            {
                root["error"] = ErrorCodes::RPCFailed;
                return true;
            }

            std::vector<MomentLikeInfo> likes;
            bool has_more = false;
            Persistence().LoadCommentLikes(request_dto.comment_id,
                                           memochat::moments::service::modules::CommentLikeFetchLimit(),
                                           likes,
                                           has_more);

            root = ToJsonValue(MomentCommentLikeResponseDto{ErrorCodes::Success,
                                                            request_dto.comment_id,
                                                            Persistence().HasCommentLike(request_dto.comment_id, uid),
                                                            static_cast<int>(likes.size()),
                                                            ToMomentLikeNames(likes),
                                                            ToMomentLikeOutputDtos(likes)});
            return true;
        });
}

} // namespace memochat::gate::services::moments
