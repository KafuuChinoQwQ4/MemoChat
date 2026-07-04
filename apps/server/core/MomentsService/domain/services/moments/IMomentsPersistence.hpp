#pragma once

#include "MomentTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace memochat::gate::services::moments
{

// Lightweight row type returned by LoadUserProfile; lives here so it is part
// of the interface contract without creating a circular include.
struct MomentUserProfileRow
{
    std::string user_id;
    std::string name;
    std::string nick;
    std::string icon;
};

class IMomentsPersistence
{
public:
    virtual ~IMomentsPersistence() = default;

    virtual bool LoadUserProfile(int uid, MomentUserProfileRow& profile) const = 0;
    virtual bool CreateMoment(const MomentInfo& moment, int64_t* moment_id) const = 0;
    virtual bool DeleteMoment(int64_t moment_id, int uid) const = 0;
    virtual bool LoadFeedCandidates(int viewer_uid,
                                    int64_t last_moment_id,
                                    int limit,
                                    int author_uid,
                                    std::vector<MomentInfo>& moments,
                                    bool& has_more) const = 0;
    virtual bool LoadMoment(int64_t moment_id, MomentInfo& moment) const = 0;
    virtual bool SetMomentLike(int64_t moment_id, int uid, bool like) const = 0;
    virtual bool HasMomentLike(int64_t moment_id, int uid) const = 0;
    virtual bool
    LoadMomentLikes(int64_t moment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more) const = 0;
    virtual bool AddComment(const MomentCommentInfo& comment) const = 0;
    virtual bool DeleteComment(int64_t comment_id, int uid) const = 0;
    virtual bool LoadComment(int64_t comment_id, MomentCommentInfo& comment) const = 0;
    virtual bool LoadComments(int64_t moment_id,
                              int64_t last_comment_id,
                              int limit,
                              std::vector<MomentCommentInfo>& comments,
                              bool& has_more) const = 0;
    virtual bool SetCommentLike(int64_t comment_id, int uid, bool like) const = 0;
    virtual bool HasCommentLike(int64_t comment_id, int uid) const = 0;
    virtual bool
    LoadCommentLikes(int64_t comment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more) const = 0;
};

} // namespace memochat::gate::services::moments
