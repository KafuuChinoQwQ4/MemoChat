#pragma once

// IMomentsPersistence.hpp already includes MomentTypes.hpp and defines
// MomentUserProfileRow, so no duplication needed here.
#include "IMomentsPersistence.hpp"

namespace memochat::gate::services::moments
{

class MomentsPersistence : public IMomentsPersistence
{
public:
    MomentsPersistence() = default; // public; no Instance()

    bool LoadUserProfile(int uid, MomentUserProfileRow& profile) const override;
    bool CreateMoment(const MomentInfo& moment, int64_t* moment_id) const override;
    bool DeleteMoment(int64_t moment_id, int uid) const override;
    bool LoadFeedCandidates(int viewer_uid,
                            int64_t last_moment_id,
                            int limit,
                            int author_uid,
                            std::vector<MomentInfo>& moments,
                            bool& has_more) const override;
    bool LoadMoment(int64_t moment_id, MomentInfo& moment) const override;
    bool SetMomentLike(int64_t moment_id, int uid, bool like) const override;
    bool HasMomentLike(int64_t moment_id, int uid) const override;
    bool
    LoadMomentLikes(int64_t moment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more) const override;
    bool AddComment(const MomentCommentInfo& comment) const override;
    bool DeleteComment(int64_t comment_id, int uid) const override;
    bool LoadComment(int64_t comment_id, MomentCommentInfo& comment) const override;
    bool LoadComments(int64_t moment_id,
                      int64_t last_comment_id,
                      int limit,
                      std::vector<MomentCommentInfo>& comments,
                      bool& has_more) const override;
    bool SetCommentLike(int64_t comment_id, int uid, bool like) const override;
    bool HasCommentLike(int64_t comment_id, int uid) const override;
    bool
    LoadCommentLikes(int64_t comment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more) const override;
};

} // namespace memochat::gate::services::moments
