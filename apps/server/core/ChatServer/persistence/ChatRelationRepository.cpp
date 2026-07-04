#include "ChatRelationRepository.hpp"

#include "PostgresMgr.hpp"

import memochat.chat.relation_repository_algorithms;

namespace
{
namespace relation_repository_modules = memochat::chat::persistence::relation_repository::modules;
} // namespace

bool ChatRelationRepository::GetUidByUserId(const std::string& user_id, int& uid)
{
    return PostgresMgr::GetInstance()->GetUidByUserId(user_id, uid);
}

std::shared_ptr<UserInfo> ChatRelationRepository::GetUserByUid(int uid)
{
    return PostgresMgr::GetInstance()->GetUser(uid);
}

bool ChatRelationRepository::RefreshDialogsForOwner(int owner_uid)
{
    return PostgresMgr::GetInstance()->RefreshDialogsForOwner(owner_uid);
}

bool ChatRelationRepository::GetDialogMetaByOwner(int owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas)
{
    return PostgresMgr::GetInstance()->GetDialogMetaByOwner(owner_uid, metas);
}

bool ChatRelationRepository::GetPrivateDialogRuntime(int owner_uid, int peer_uid, DialogRuntimeInfo& runtime)
{
    return PostgresMgr::GetInstance()->GetPrivateDialogRuntime(owner_uid, peer_uid, runtime);
}

bool ChatRelationRepository::GetGroupDialogRuntime(int owner_uid, int64_t group_id, DialogRuntimeInfo& runtime)
{
    return PostgresMgr::GetInstance()->GetGroupDialogRuntime(owner_uid, group_id, runtime);
}

bool ChatRelationRepository::GetUserGroupList(int uid, std::vector<std::shared_ptr<GroupInfo>>& group_list)
{
    return PostgresMgr::GetInstance()->GetUserGroupList(uid, group_list);
}

bool ChatRelationRepository::GetGroupById(int64_t group_id, std::shared_ptr<GroupInfo>& group_info)
{
    return PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
}

bool ChatRelationRepository::GetPendingGroupApplyForReviewer(int reviewer_uid,
                                                             std::vector<std::shared_ptr<GroupApplyInfo>>& applies,
                                                             int limit)
{
    return PostgresMgr::GetInstance()->GetPendingGroupApplyForReviewer(reviewer_uid, applies, limit);
}

bool ChatRelationRepository::GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id)
{
    return PostgresMgr::GetInstance()->GetGroupIdByCode(group_code, out_group_id);
}

bool ChatRelationRepository::GetGroupMemberList(int64_t group_id,
                                                std::vector<std::shared_ptr<GroupMemberInfo>>& member_list)
{
    return PostgresMgr::GetInstance()->GetGroupMemberList(group_id, member_list);
}

bool ChatRelationRepository::AddFriendApply(int from, int to)
{
    return PostgresMgr::GetInstance()->AddFriendApply(from, to);
}

bool ChatRelationRepository::ReplaceApplyTags(int from, int to, const std::vector<std::string>& tags)
{
    return PostgresMgr::GetInstance()->ReplaceApplyTags(from, to, tags);
}

bool ChatRelationRepository::AuthFriendApply(int from, int to)
{
    return PostgresMgr::GetInstance()->AuthFriendApply(from, to);
}

bool ChatRelationRepository::AddFriend(int from, int to, const std::string& back_name)
{
    return PostgresMgr::GetInstance()->AddFriend(from, to, back_name);
}

std::vector<std::string> ChatRelationRepository::GetApplyTags(int from, int to)
{
    return PostgresMgr::GetInstance()->GetApplyTags(from, to);
}

bool ChatRelationRepository::ReplaceFriendTags(int self_id, int friend_id, const std::vector<std::string>& tags)
{
    return PostgresMgr::GetInstance()->ReplaceFriendTags(self_id, friend_id, tags);
}

bool ChatRelationRepository::DeleteFriend(int from, int to)
{
    return PostgresMgr::GetInstance()->DeleteFriend(from, to);
}

bool ChatRelationRepository::IsPrivateFriend(int self_id, int friend_id)
{
    if (!relation_repository_modules::ShouldQueryPrivateFriendship(self_id, friend_id))
    {
        return false;
    }
    return PostgresMgr::GetInstance()->IsFriend(self_id, friend_id);
}

std::vector<int> ChatRelationRepository::FilterFriendUids(int viewer_uid, const std::vector<int>& author_uids)
{
    if (!relation_repository_modules::ShouldFilterFriendUids(viewer_uid, author_uids.empty()))
    {
        return {};
    }
    return PostgresMgr::GetInstance()->FilterFriendUids(viewer_uid, author_uids);
}

bool ChatRelationRepository::IsGroupMember(int64_t group_id, int uid)
{
    if (!relation_repository_modules::ShouldQueryGroupMembership(group_id, uid))
    {
        return false;
    }
    return PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid);
}

bool ChatRelationRepository::CreateGroup(int owner_uid,
                                         const std::string& name,
                                         const std::string& announcement,
                                         int member_limit,
                                         const std::vector<int>& initial_members,
                                         int64_t& out_group_id,
                                         std::string& out_group_code)
{
    return PostgresMgr::GetInstance()
        ->CreateGroup(owner_uid, name, announcement, member_limit, initial_members, out_group_id, out_group_code);
}

bool ChatRelationRepository::InviteGroupMember(int64_t group_id,
                                               int inviter_uid,
                                               int target_uid,
                                               const std::string& reason)
{
    return PostgresMgr::GetInstance()->InviteGroupMember(group_id, inviter_uid, target_uid, reason);
}

bool ChatRelationRepository::ApplyJoinGroup(int64_t group_id, int applicant_uid, const std::string& reason)
{
    return PostgresMgr::GetInstance()->ApplyJoinGroup(group_id, applicant_uid, reason);
}

bool ChatRelationRepository::ReviewGroupApply(int64_t apply_id,
                                              int reviewer_uid,
                                              bool agree,
                                              std::shared_ptr<GroupApplyInfo>& apply_info)
{
    return PostgresMgr::GetInstance()->ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info);
}

bool ChatRelationRepository::GetUserRoleInGroup(int64_t group_id, int uid, int& role)
{
    return PostgresMgr::GetInstance()->GetUserRoleInGroup(group_id, uid, role);
}

bool ChatRelationRepository::UpdateGroupAnnouncement(int64_t group_id,
                                                     int operator_uid,
                                                     const std::string& announcement)
{
    return PostgresMgr::GetInstance()->UpdateGroupAnnouncement(group_id, operator_uid, announcement);
}

bool ChatRelationRepository::UpdateGroupIcon(int64_t group_id, int operator_uid, const std::string& icon)
{
    return PostgresMgr::GetInstance()->UpdateGroupIcon(group_id, operator_uid, icon);
}

bool ChatRelationRepository::SetGroupAdmin(int64_t group_id,
                                           int operator_uid,
                                           int target_uid,
                                           bool is_admin,
                                           int64_t permission_bits)
{
    return PostgresMgr::GetInstance()->SetGroupAdmin(group_id, operator_uid, target_uid, is_admin, permission_bits);
}

bool ChatRelationRepository::MuteGroupMember(int64_t group_id, int operator_uid, int target_uid, int64_t mute_until)
{
    return PostgresMgr::GetInstance()->MuteGroupMember(group_id, operator_uid, target_uid, mute_until);
}

bool ChatRelationRepository::KickGroupMember(int64_t group_id, int operator_uid, int target_uid)
{
    return PostgresMgr::GetInstance()->KickGroupMember(group_id, operator_uid, target_uid);
}

bool ChatRelationRepository::QuitGroup(int64_t group_id, int uid)
{
    return PostgresMgr::GetInstance()->QuitGroup(group_id, uid);
}

bool ChatRelationRepository::DissolveGroup(int64_t group_id, int operator_uid)
{
    return PostgresMgr::GetInstance()->DissolveGroup(group_id, operator_uid);
}

bool ChatRelationRepository::UpsertDialogDraft(int owner_uid,
                                               const std::string& dialog_type,
                                               int peer_uid,
                                               int64_t group_id,
                                               const std::string& draft_text)
{
    return PostgresMgr::GetInstance()->UpsertDialogDraft(owner_uid, dialog_type, peer_uid, group_id, draft_text);
}

bool ChatRelationRepository::UpsertDialogMuteState(int owner_uid,
                                                   const std::string& dialog_type,
                                                   int peer_uid,
                                                   int64_t group_id,
                                                   int mute_state)
{
    return PostgresMgr::GetInstance()->UpsertDialogMuteState(owner_uid, dialog_type, peer_uid, group_id, mute_state);
}

bool ChatRelationRepository::UpsertDialogPinned(int owner_uid,
                                                const std::string& dialog_type,
                                                int peer_uid,
                                                int64_t group_id,
                                                int pinned_rank)
{
    return PostgresMgr::GetInstance()->UpsertDialogPinned(owner_uid, dialog_type, peer_uid, group_id, pinned_rank);
}
