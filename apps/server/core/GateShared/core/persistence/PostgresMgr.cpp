#include "PostgresMgr.hpp"

import memochat.gate.postgres_mgr_algorithms;

namespace
{
namespace postgres_mgr_modules = memochat::gate::postgres_mgr::modules;

// Lock the pure-forwarding facade surface: every PostgresMgr method forwards
// directly to the single shared PostgresDao member, so the per-domain method
// counts and total must match the module contract.
static_assert(postgres_mgr_modules::IsCompleteForwardingSurface(postgres_mgr_modules::AccountForwardCount(),
                                                                postgres_mgr_modules::CallForwardCount(),
                                                                postgres_mgr_modules::MediaForwardCount(),
                                                                postgres_mgr_modules::MomentForwardCount()));
static_assert(postgres_mgr_modules::ForwardingSurfaceCount() == 35u);
} // namespace

PostgresMgr::~PostgresMgr()
{
}

int PostgresMgr::RegUser(const std::string& name,
                         const std::string& email,
                         const std::string& pwd,
                         const std::string& icon)
{
    return _dao.RegUserTransaction(name, email, pwd, icon);
}

bool PostgresMgr::CheckEmail(const std::string& name, const std::string& email)
{
    return _dao.CheckEmail(name, email);
}

bool PostgresMgr::UpdatePwd(const std::string& email, const std::string& pwd)
{
    return _dao.UpdatePwd(email, pwd);
}

bool PostgresMgr::IssueRefreshToken(int uid,
                                    const std::string& selector,
                                    const std::string& verifier_hash,
                                    int ttl_seconds,
                                    const std::string& user_agent,
                                    const std::string& ip_hash)
{
    return _dao.IssueRefreshToken(uid, selector, verifier_hash, ttl_seconds, user_agent, ip_hash);
}

RefreshTokenRotationStatus PostgresMgr::RotateRefreshToken(const std::string& selector,
                                                           const std::string& verifier,
                                                           const std::string& replacement_selector,
                                                           const std::string& replacement_verifier_hash,
                                                           int ttl_seconds,
                                                           const std::string& user_agent,
                                                           const std::string& ip_hash,
                                                           int& uid)
{
    return _dao.RotateRefreshToken(selector,
                                   verifier,
                                   replacement_selector,
                                   replacement_verifier_hash,
                                   ttl_seconds,
                                   user_agent,
                                   ip_hash,
                                   uid);
}

bool PostgresMgr::RevokeRefreshToken(const std::string& selector, const std::string& verifier, int& uid)
{
    return _dao.RevokeRefreshToken(selector, verifier, uid);
}

bool PostgresMgr::RevokeAllRefreshTokensForUid(int uid)
{
    return _dao.RevokeAllRefreshTokensForUid(uid);
}

bool PostgresMgr::UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon)
{
    return _dao.UpdateUserProfile(uid, nick, desc, icon);
}
PostgresMgr::PostgresMgr()
{
}

bool PostgresMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    return _dao.CheckPwd(email, pwd, userInfo);
}

std::string PostgresMgr::GetUserPublicId(int uid)
{
    return _dao.GetUserPublicId(uid);
}

bool PostgresMgr::GetCallUserProfile(int uid, CallUserProfile& profile)
{
    return _dao.GetCallUserProfile(uid, profile);
}

bool PostgresMgr::IsFriend(int uid, int peer_uid)
{
    return _dao.IsFriend(uid, peer_uid);
}

bool PostgresMgr::UpsertCallSession(const CallSessionInfo& session)
{
    return _dao.UpsertCallSession(session);
}

bool PostgresMgr::GetCallSession(const std::string& call_id, CallSessionInfo& session)
{
    return _dao.GetCallSession(call_id, session);
}

bool PostgresMgr::InsertMediaAsset(const MediaAssetInfo& asset)
{
    return _dao.InsertMediaAsset(asset);
}

bool PostgresMgr::GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset)
{
    return _dao.GetMediaAssetByKey(media_key, asset);
}

bool PostgresMgr::GrantMediaAccess(int64_t media_id, int grantee_uid, const std::string& scope, int64_t created_at_ms)
{
    return _dao.GrantMediaAccess(media_id, grantee_uid, scope, created_at_ms);
}

bool PostgresMgr::GrantMediaGroupAccess(int64_t media_id,
                                        int64_t group_id,
                                        int owner_uid,
                                        const std::string& scope,
                                        int64_t created_at_ms)
{
    return _dao.GrantMediaGroupAccess(media_id, group_id, owner_uid, scope, created_at_ms);
}

bool PostgresMgr::HasMediaAccess(int64_t media_id, int uid)
{
    return _dao.HasMediaAccess(media_id, uid);
}

bool PostgresMgr::GetUserInfo(int uid, UserInfo& user_info)
{
    return _dao.GetUserInfo(uid, user_info);
}

bool PostgresMgr::TestProcedure(const std::string& email, int& uid, std::string& name)
{
    return _dao.TestProcedure(email, uid, name);
}

bool PostgresMgr::AddMoment(const MomentInfo& moment, int64_t* moment_id)
{
    return _dao.AddMoment(moment, moment_id);
}

bool PostgresMgr::GetMomentsFeed(int viewer_uid,
                                 int64_t last_moment_id,
                                 int limit,
                                 int author_uid,
                                 std::vector<MomentInfo>& moments,
                                 bool& has_more)
{
    return _dao.GetMomentsFeed(viewer_uid, last_moment_id, limit, author_uid, moments, has_more);
}

bool PostgresMgr::GetMomentsFeedCandidates(int viewer_uid,
                                           int64_t last_moment_id,
                                           int limit,
                                           int author_uid,
                                           std::vector<MomentInfo>& moments,
                                           bool& has_more)
{
    return _dao.GetMomentsFeedCandidates(viewer_uid, last_moment_id, limit, author_uid, moments, has_more);
}

bool PostgresMgr::CanViewMoment(int viewer_uid, const MomentInfo& moment)
{
    return _dao.CanViewMoment(viewer_uid, moment);
}

bool PostgresMgr::GetMomentById(int64_t moment_id, MomentInfo& moment)
{
    return _dao.GetMomentById(moment_id, moment);
}

bool PostgresMgr::DeleteMoment(int64_t moment_id, int uid)
{
    return _dao.DeleteMoment(moment_id, uid);
}

bool PostgresMgr::AddMomentLike(int64_t moment_id, int uid)
{
    return _dao.AddMomentLike(moment_id, uid);
}

bool PostgresMgr::RemoveMomentLike(int64_t moment_id, int uid)
{
    return _dao.RemoveMomentLike(moment_id, uid);
}

bool PostgresMgr::HasLikedMoment(int64_t moment_id, int uid)
{
    return _dao.HasLikedMoment(moment_id, uid);
}

bool PostgresMgr::GetMomentLikes(int64_t moment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more)
{
    return _dao.GetMomentLikes(moment_id, limit, likes, has_more);
}

bool PostgresMgr::AddMomentComment(const MomentCommentInfo& comment)
{
    return _dao.AddMomentComment(comment);
}

bool PostgresMgr::DeleteMomentComment(int64_t comment_id, int uid)
{
    return _dao.DeleteMomentComment(comment_id, uid);
}

bool PostgresMgr::GetMomentCommentById(int64_t comment_id, MomentCommentInfo& comment)
{
    return _dao.GetMomentCommentById(comment_id, comment);
}

bool PostgresMgr::GetMomentComments(int64_t moment_id,
                                    int64_t last_comment_id,
                                    int limit,
                                    std::vector<MomentCommentInfo>& comments,
                                    bool& has_more)
{
    return _dao.GetMomentComments(moment_id, last_comment_id, limit, comments, has_more);
}

bool PostgresMgr::AddMomentCommentLike(int64_t comment_id, int uid)
{
    return _dao.AddMomentCommentLike(comment_id, uid);
}

bool PostgresMgr::RemoveMomentCommentLike(int64_t comment_id, int uid)
{
    return _dao.RemoveMomentCommentLike(comment_id, uid);
}

bool PostgresMgr::HasLikedMomentComment(int64_t comment_id, int uid)
{
    return _dao.HasLikedMomentComment(comment_id, uid);
}

bool PostgresMgr::GetMomentCommentLikes(int64_t comment_id,
                                        int limit,
                                        std::vector<MomentLikeInfo>& likes,
                                        bool& has_more)
{
    return _dao.GetMomentCommentLikes(comment_id, limit, likes, has_more);
}
