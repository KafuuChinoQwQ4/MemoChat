export module memochat.gate.postgres_mgr_algorithms;

// GateInfraCore shared Postgres manager facade surface contract.
// PostgresMgr is a pure forwarding facade: every public method forwards
// directly to the single shared PostgresDao member. This module locks the
// facade forwarding surface shape (per-domain method counts + total) at
// compile time. It exports primitive counts only; it does NOT export
// PostgresDao, pqxx types, UserInfo/MomentInfo/Call/Media DTOs, SQL, row
// mapping, connection pool lifecycle, std::string, vectors, or database I/O.
export namespace memochat::gate::postgres_mgr::modules
{
constexpr unsigned AccountForwardCount()
{
    // RegUser, CheckEmail, UpdatePwd, UpdateUserProfile, CheckPwd,
    // IssueRefreshToken, RotateRefreshToken, ResolveActiveRefreshTokenUserId, RevokeRefreshToken,
    // RevokeAllRefreshTokensForUid, GetUserPublicId, GetCallUserProfile,
    // IsFriend, GetUserInfo, GetR18AccessPolicy, AttestAdultForR18,
    // TestProcedure
    return 17;
}

constexpr unsigned CallForwardCount()
{
    // UpsertCallSession, GetCallSession
    return 2;
}

constexpr unsigned MediaForwardCount()
{
    // InsertMediaAsset, GetMediaAssetByKey
    return 2;
}

constexpr unsigned MomentForwardCount()
{
    // AddMoment, GetMomentsFeed, GetMomentsFeedCandidates, CanViewMoment,
    // GetMomentById, DeleteMoment, AddMomentLike, RemoveMomentLike,
    // HasLikedMoment, GetMomentLikes, AddMomentComment, DeleteMomentComment,
    // GetMomentComments, AddMomentCommentLike, RemoveMomentCommentLike,
    // HasLikedMomentComment, GetMomentCommentLikes
    return 17;
}

constexpr unsigned ForwardingSurfaceCount()
{
    return AccountForwardCount() + CallForwardCount() + MediaForwardCount() + MomentForwardCount();
}

constexpr bool IsCompleteForwardingSurface(unsigned account_count,
                                           unsigned call_count,
                                           unsigned media_count,
                                           unsigned moment_count)
{
    return account_count == AccountForwardCount() && call_count == CallForwardCount() &&
           media_count == MediaForwardCount() && moment_count == MomentForwardCount();
}
} // namespace memochat::gate::postgres_mgr::modules
