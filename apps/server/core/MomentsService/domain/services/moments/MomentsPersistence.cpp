#include "services/moments/MomentsPersistence.hpp"

#include "PostgresMgr.hpp"

namespace memochat::gate::services::moments
{

bool MomentsPersistence::LoadUserProfile(int uid, MomentUserProfileRow& profile) const
{
    UserInfo user_info;
    if (!PostgresMgr::GetInstance()->GetUserInfo(uid, user_info))
    {
        return false;
    }
    profile.user_id = user_info.user_id;
    profile.name = user_info.name;
    profile.nick = user_info.nick;
    profile.icon = user_info.icon;
    return true;
}

bool MomentsPersistence::CreateMoment(const MomentInfo& moment, int64_t* moment_id) const
{
    return PostgresMgr::GetInstance()->AddMoment(moment, moment_id);
}

bool MomentsPersistence::DeleteMoment(int64_t moment_id, int uid) const
{
    return PostgresMgr::GetInstance()->DeleteMoment(moment_id, uid);
}

bool MomentsPersistence::LoadFeedCandidates(int viewer_uid,
                                            int64_t last_moment_id,
                                            int limit,
                                            int author_uid,
                                            std::vector<MomentInfo>& moments,
                                            bool& has_more) const
{
    return PostgresMgr::GetInstance()
        ->GetMomentsFeedCandidates(viewer_uid, last_moment_id, limit, author_uid, moments, has_more);
}

bool MomentsPersistence::LoadMoment(int64_t moment_id, MomentInfo& moment) const
{
    return PostgresMgr::GetInstance()->GetMomentById(moment_id, moment);
}

bool MomentsPersistence::SetMomentLike(int64_t moment_id, int uid, bool like) const
{
    return like ? PostgresMgr::GetInstance()->AddMomentLike(moment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentLike(moment_id, uid);
}

bool MomentsPersistence::HasMomentLike(int64_t moment_id, int uid) const
{
    return PostgresMgr::GetInstance()->HasLikedMoment(moment_id, uid);
}

bool MomentsPersistence::LoadMomentLikes(int64_t moment_id,
                                         int limit,
                                         std::vector<MomentLikeInfo>& likes,
                                         bool& has_more) const
{
    return PostgresMgr::GetInstance()->GetMomentLikes(moment_id, limit, likes, has_more);
}

bool MomentsPersistence::AddComment(const MomentCommentInfo& comment) const
{
    return PostgresMgr::GetInstance()->AddMomentComment(comment);
}

bool MomentsPersistence::DeleteComment(int64_t comment_id, int uid) const
{
    return PostgresMgr::GetInstance()->DeleteMomentComment(comment_id, uid);
}

bool MomentsPersistence::LoadComment(int64_t comment_id, MomentCommentInfo& comment) const
{
    return PostgresMgr::GetInstance()->GetMomentCommentById(comment_id, comment);
}

bool MomentsPersistence::LoadComments(int64_t moment_id,
                                      int64_t last_comment_id,
                                      int limit,
                                      std::vector<MomentCommentInfo>& comments,
                                      bool& has_more) const
{
    return PostgresMgr::GetInstance()->GetMomentComments(moment_id, last_comment_id, limit, comments, has_more);
}

bool MomentsPersistence::SetCommentLike(int64_t comment_id, int uid, bool like) const
{
    return like ? PostgresMgr::GetInstance()->AddMomentCommentLike(comment_id, uid)
                : PostgresMgr::GetInstance()->RemoveMomentCommentLike(comment_id, uid);
}

bool MomentsPersistence::HasCommentLike(int64_t comment_id, int uid) const
{
    return PostgresMgr::GetInstance()->HasLikedMomentComment(comment_id, uid);
}

bool MomentsPersistence::LoadCommentLikes(int64_t comment_id,
                                          int limit,
                                          std::vector<MomentLikeInfo>& likes,
                                          bool& has_more) const
{
    return PostgresMgr::GetInstance()->GetMomentCommentLikes(comment_id, limit, likes, has_more);
}

} // namespace memochat::gate::services::moments
