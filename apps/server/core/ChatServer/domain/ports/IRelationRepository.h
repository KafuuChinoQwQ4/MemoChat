#pragma once

#include "data.h"

#include <memory>
#include <string>
#include <vector>

class IRelationRepository
{
public:
    virtual ~IRelationRepository() = default;

    virtual bool GetUidByUserId(const std::string& user_id, int& uid) = 0;
    virtual std::shared_ptr<UserInfo> GetUserByUid(int uid) = 0;
    virtual bool RefreshDialogsForOwner(int owner_uid) = 0;
    virtual bool GetDialogMetaByOwner(int owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas) = 0;
    virtual bool GetPrivateDialogRuntime(int owner_uid, int peer_uid, DialogRuntimeInfo& runtime) = 0;
    virtual bool GetGroupDialogRuntime(int owner_uid, int64_t group_id, DialogRuntimeInfo& runtime) = 0;
    virtual bool GetUserGroupList(int uid, std::vector<std::shared_ptr<GroupInfo>>& group_list) = 0;
    virtual bool GetGroupById(int64_t group_id, std::shared_ptr<GroupInfo>& group_info) = 0;
    virtual bool GetPendingGroupApplyForReviewer(int reviewer_uid,
                                                 std::vector<std::shared_ptr<GroupApplyInfo>>& applies,
                                                 int limit = 20) = 0;
    virtual bool GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id) = 0;
    virtual bool GetGroupMemberList(int64_t group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list) = 0;
    virtual bool AddFriendApply(int from, int to) = 0;
    virtual bool ReplaceApplyTags(int from, int to, const std::vector<std::string>& tags) = 0;
    virtual bool AuthFriendApply(int from, int to) = 0;
    virtual bool AddFriend(int from, int to, const std::string& back_name) = 0;
    virtual std::vector<std::string> GetApplyTags(int from, int to) = 0;
    virtual bool ReplaceFriendTags(int self_id, int friend_id, const std::vector<std::string>& tags) = 0;
    virtual bool DeleteFriend(int from, int to) = 0;
    virtual bool IsPrivateFriend(int self_id, int friend_id) = 0;
    virtual bool IsGroupMember(int64_t group_id, int uid) = 0;
    virtual bool CreateGroup(int owner_uid,
                             const std::string& name,
                             const std::string& announcement,
                             int member_limit,
                             const std::vector<int>& initial_members,
                             int64_t& out_group_id,
                             std::string& out_group_code) = 0;
    virtual bool InviteGroupMember(int64_t group_id, int inviter_uid, int target_uid, const std::string& reason) = 0;
    virtual bool ApplyJoinGroup(int64_t group_id, int applicant_uid, const std::string& reason) = 0;
    virtual bool
    ReviewGroupApply(int64_t apply_id, int reviewer_uid, bool agree, std::shared_ptr<GroupApplyInfo>& apply_info) = 0;
    virtual bool GetUserRoleInGroup(int64_t group_id, int uid, int& role) = 0;
    virtual bool UpdateGroupAnnouncement(int64_t group_id, int operator_uid, const std::string& announcement) = 0;
    virtual bool UpdateGroupIcon(int64_t group_id, int operator_uid, const std::string& icon) = 0;
    virtual bool
    SetGroupAdmin(int64_t group_id, int operator_uid, int target_uid, bool is_admin, int64_t permission_bits = 0) = 0;
    virtual bool MuteGroupMember(int64_t group_id, int operator_uid, int target_uid, int64_t mute_until) = 0;
    virtual bool KickGroupMember(int64_t group_id, int operator_uid, int target_uid) = 0;
    virtual bool QuitGroup(int64_t group_id, int uid) = 0;
    virtual bool DissolveGroup(int64_t group_id, int operator_uid) = 0;
    virtual bool UpsertDialogDraft(int owner_uid,
                                   const std::string& dialog_type,
                                   int peer_uid,
                                   int64_t group_id,
                                   const std::string& draft_text) = 0;
    virtual bool UpsertDialogMuteState(int owner_uid,
                                       const std::string& dialog_type,
                                       int peer_uid,
                                       int64_t group_id,
                                       int mute_state) = 0;
    virtual bool UpsertDialogPinned(int owner_uid,
                                    const std::string& dialog_type,
                                    int peer_uid,
                                    int64_t group_id,
                                    int pinned_rank) = 0;
};
