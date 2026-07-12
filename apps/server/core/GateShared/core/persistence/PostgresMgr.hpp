#pragma once
#include "const.hpp"
#include "PostgresDao.hpp"
class PostgresMgr : public Singleton<PostgresMgr>
{
    friend class Singleton<PostgresMgr>;

public:
    ~PostgresMgr();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] const std::string& StartupError() const noexcept;

    int RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& email, const std::string& pwd);
    bool IssueRefreshToken(int uid,
                           const std::string& selector,
                           const std::string& verifier_hash,
                           int ttl_seconds,
                           const std::string& user_agent,
                           const std::string& ip_hash);
    RefreshTokenRotationStatus RotateRefreshToken(const std::string& selector,
                                                  const std::string& verifier,
                                                  const std::string& replacement_selector,
                                                  const std::string& replacement_verifier_hash,
                                                  int ttl_seconds,
                                                  const std::string& user_agent,
                                                  const std::string& ip_hash,
                                                  int& uid);
    bool ResolveActiveRefreshTokenUserId(const std::string& selector, const std::string& verifier, int& uid);
    bool RevokeRefreshToken(const std::string& selector, const std::string& verifier, int& uid);
    bool RevokeAllRefreshTokensForUid(int uid);
    bool UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon);
    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
    std::string GetUserPublicId(int uid);
    bool GetCallUserProfile(int uid, CallUserProfile& profile);
    bool IsFriend(int uid, int peer_uid);
    bool UpsertCallSession(const CallSessionInfo& session);
    bool GetCallSession(const std::string& call_id, CallSessionInfo& session);
    bool InsertMediaAsset(const MediaAssetInfo& asset);
    bool GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset);
    bool GrantMediaAccess(int64_t media_id, int grantee_uid, const std::string& scope, int64_t created_at_ms);
    bool GrantMediaGroupAccess(int64_t media_id,
                               int64_t group_id,
                               int owner_uid,
                               const std::string& scope,
                               int64_t created_at_ms);
    bool HasMediaAccess(int64_t media_id, int uid);
    bool GetUserInfo(int uid, UserInfo& user_info);
    bool GetR18AccessPolicy(int uid, R18AccessPolicyInfo& policy);
    bool AttestAdultForR18(int uid, int64_t attested_at_ms, R18AccessPolicyInfo& policy);
    bool TestProcedure(const std::string& email, int& uid, std::string& name);

    // Moments operations
    bool AddMoment(const MomentInfo& moment, int64_t* moment_id = nullptr);
    bool GetMomentsFeed(int viewer_uid,
                        int64_t last_moment_id,
                        int limit,
                        int author_uid,
                        std::vector<MomentInfo>& moments,
                        bool& has_more);
    bool GetMomentsFeedCandidates(int viewer_uid,
                                  int64_t last_moment_id,
                                  int limit,
                                  int author_uid,
                                  std::vector<MomentInfo>& moments,
                                  bool& has_more);
    bool CanViewMoment(int viewer_uid, const MomentInfo& moment);
    bool GetMomentById(int64_t moment_id, MomentInfo& moment);
    bool DeleteMoment(int64_t moment_id, int uid);
    bool AddMomentLike(int64_t moment_id, int uid);
    bool RemoveMomentLike(int64_t moment_id, int uid);
    bool HasLikedMoment(int64_t moment_id, int uid);
    bool GetMomentLikes(int64_t moment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more);
    bool AddMomentComment(const MomentCommentInfo& comment);
    bool DeleteMomentComment(int64_t comment_id, int uid);
    bool GetMomentCommentById(int64_t comment_id, MomentCommentInfo& comment);
    bool GetMomentComments(int64_t moment_id,
                           int64_t last_comment_id,
                           int limit,
                           std::vector<MomentCommentInfo>& comments,
                           bool& has_more);
    bool AddMomentCommentLike(int64_t comment_id, int uid);
    bool RemoveMomentCommentLike(int64_t comment_id, int uid);
    bool HasLikedMomentComment(int64_t comment_id, int uid);
    bool GetMomentCommentLikes(int64_t comment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more);

private:
    PostgresMgr();
    PostgresDao _dao;
};
