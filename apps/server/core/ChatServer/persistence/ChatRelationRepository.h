#pragma once

#include "ports/IRelationRepository.h"

class ChatRelationRepository : public IRelationRepository
{
public:
    bool GetUidByUserId(const std::string& user_id, int& uid) override;
    std::shared_ptr<UserInfo> GetUserByUid(int uid) override;
    bool RefreshDialogsForOwner(int owner_uid) override;
    bool GetDialogMetaByOwner(int owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas) override;
    bool GetPrivateDialogRuntime(int owner_uid, int peer_uid, DialogRuntimeInfo& runtime) override;
    bool GetGroupDialogRuntime(int owner_uid, int64_t group_id, DialogRuntimeInfo& runtime) override;
    bool GetUserGroupList(int uid, std::vector<std::shared_ptr<GroupInfo>>& group_list) override;
    bool GetGroupById(int64_t group_id, std::shared_ptr<GroupInfo>& group_info) override;
    bool GetPendingGroupApplyForReviewer(int reviewer_uid,
                                         std::vector<std::shared_ptr<GroupApplyInfo>>& applies,
                                         int limit = 20) override;
    bool GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id) override;
    bool GetGroupMemberList(int64_t group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list) override;
    bool AddFriendApply(int from, int to) override;
    bool ReplaceApplyTags(int from, int to, const std::vector<std::string>& tags) override;
    bool AuthFriendApply(int from, int to) override;
    bool AddFriend(int from, int to, const std::string& back_name) override;
    std::vector<std::string> GetApplyTags(int from, int to) override;
    bool ReplaceFriendTags(int self_id, int friend_id, const std::vector<std::string>& tags) override;
    bool DeleteFriend(int from, int to) override;
    bool IsPrivateFriend(int self_id, int friend_id) override;
    bool IsGroupMember(int64_t group_id, int uid) override;
    bool CreateGroup(int owner_uid,
                     const std::string& name,
                     const std::string& announcement,
                     int member_limit,
                     const std::vector<int>& initial_members,
                     int64_t& out_group_id,
                     std::string& out_group_code) override;
    bool InviteGroupMember(int64_t group_id, int inviter_uid, int target_uid, const std::string& reason) override;
    bool ApplyJoinGroup(int64_t group_id, int applicant_uid, const std::string& reason) override;
    bool ReviewGroupApply(int64_t apply_id,
                          int reviewer_uid,
                          bool agree,
                          std::shared_ptr<GroupApplyInfo>& apply_info) override;
    bool GetUserRoleInGroup(int64_t group_id, int uid, int& role) override;
    bool UpdateGroupAnnouncement(int64_t group_id, int operator_uid, const std::string& announcement) override;
    bool UpdateGroupIcon(int64_t group_id, int operator_uid, const std::string& icon) override;
    bool SetGroupAdmin(int64_t group_id,
                       int operator_uid,
                       int target_uid,
                       bool is_admin,
                       int64_t permission_bits = 0) override;
    bool MuteGroupMember(int64_t group_id, int operator_uid, int target_uid, int64_t mute_until) override;
    bool KickGroupMember(int64_t group_id, int operator_uid, int target_uid) override;
    bool QuitGroup(int64_t group_id, int uid) override;
    bool DissolveGroup(int64_t group_id, int operator_uid) override;
    bool UpsertDialogDraft(int owner_uid,
                           const std::string& dialog_type,
                           int peer_uid,
                           int64_t group_id,
                           const std::string& draft_text) override;
    bool UpsertDialogMuteState(int owner_uid,
                               const std::string& dialog_type,
                               int peer_uid,
                               int64_t group_id,
                               int mute_state) override;
    bool UpsertDialogPinned(int owner_uid,
                            const std::string& dialog_type,
                            int peer_uid,
                            int64_t group_id,
                            int pinned_rank) override;
};
