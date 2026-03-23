#include "PostgresMgr.h"


PostgresMgr::~PostgresMgr() {

}

int PostgresMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _dao.RegUser(name, email, pwd);
}

bool PostgresMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}

bool PostgresMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

PostgresMgr::PostgresMgr() {
}

bool PostgresMgr::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(name, pwd, userInfo);
}

bool PostgresMgr::AddFriendApply(const int& from, const int& to)
{
	return _dao.AddFriendApply(from, to);
}

bool PostgresMgr::AuthFriendApply(const int& from, const int& to) {
	return _dao.AuthFriendApply(from, to);
}

bool PostgresMgr::AddFriend(const int& from, const int& to, std::string back_name) {
	return _dao.AddFriend(from, to, back_name);
}

bool PostgresMgr::ReplaceApplyTags(const int& from, const int& to, const std::vector<std::string>& tags) {
	return _dao.ReplaceApplyTags(from, to, tags);
}

bool PostgresMgr::ReplaceFriendTags(const int& self_id, const int& friend_id, const std::vector<std::string>& tags) {
	return _dao.ReplaceFriendTags(self_id, friend_id, tags);
}

std::vector<std::string> PostgresMgr::GetApplyTags(const int& from, const int& to) {
	return _dao.GetApplyTags(from, to);
}

std::vector<std::string> PostgresMgr::GetFriendTags(const int& self_id, const int& friend_id) {
	return _dao.GetFriendTags(self_id, friend_id);
}
std::shared_ptr<UserInfo> PostgresMgr::GetUser(int uid)
{
	return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> PostgresMgr::GetUser(std::string name)
{
	return _dao.GetUser(name);
}

bool PostgresMgr::GetUidByUserId(const std::string& user_id, int& uid) {
	return _dao.GetUidByUserId(user_id, uid);
}

bool PostgresMgr::GetApplyList(int touid, 
	std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {

	return _dao.GetApplyList(touid, applyList, begin, limit);
}

bool PostgresMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info) {
	return _dao.GetFriendList(self_id, user_info);
}

bool PostgresMgr::SavePrivateMessage(const PrivateMessageInfo& msg) {
	return _dao.SavePrivateMessage(msg);
}

bool PostgresMgr::EnqueueChatOutboxEvent(const ChatOutboxEventInfo& event) {
	return _dao.EnqueueChatOutboxEvent(event);
}

bool PostgresMgr::GetPendingChatOutboxEvents(int limit, std::vector<ChatOutboxEventInfo>& events) {
	return _dao.GetPendingChatOutboxEvents(limit, events);
}

bool PostgresMgr::MarkChatOutboxEventPublished(int64_t id, int64_t published_at_ms) {
	return _dao.MarkChatOutboxEventPublished(id, published_at_ms);
}

bool PostgresMgr::MarkChatOutboxEventRetry(int64_t id, int retry_count, int64_t next_retry_at_ms, const std::string& last_error, bool terminal_error) {
	return _dao.MarkChatOutboxEventRetry(id, retry_count, next_retry_at_ms, last_error, terminal_error);
}

bool PostgresMgr::ExpediteChatOutboxEventRetry(int64_t id) {
	return _dao.ExpediteChatOutboxEventRetry(id);
}

bool PostgresMgr::GetPrivateHistory(const int& uid, const int& peer_uid, const int64_t& before_ts, const std::string& before_msg_id, const int& limit,
	std::vector<std::shared_ptr<PrivateMessageInfo>>& messages, bool& has_more) {
	return _dao.GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
}

bool PostgresMgr::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message) {
	return _dao.GetPrivateMessageByMsgId(msg_id, message);
}

bool PostgresMgr::UpdatePrivateMessageContent(const int& uid, const int& peer_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	return _dao.UpdatePrivateMessageContent(uid, peer_uid, msg_id, content, edited_at_ms);
}

bool PostgresMgr::RevokePrivateMessage(const int& uid, const int& peer_uid, const std::string& msg_id,
	int64_t deleted_at_ms) {
	return _dao.RevokePrivateMessage(uid, peer_uid, msg_id, deleted_at_ms);
}

bool PostgresMgr::UpsertPrivateReadState(const int& uid, const int& peer_uid, const int64_t& read_ts) {
	return _dao.UpsertPrivateReadState(uid, peer_uid, read_ts);
}

bool PostgresMgr::IsFriend(const int& self_id, const int& friend_id) {
	return _dao.IsFriend(self_id, friend_id);
}

bool PostgresMgr::CreateGroup(const int& owner_uid, const std::string& name, const std::string& announcement,
	const int& member_limit, const std::vector<int>& initial_members, int64_t& out_group_id, std::string& out_group_code) {
	return _dao.CreateGroup(owner_uid, name, announcement, member_limit, initial_members, out_group_id, out_group_code);
}

bool PostgresMgr::GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id) {
	return _dao.GetGroupIdByCode(group_code, out_group_id);
}

bool PostgresMgr::GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list) {
	return _dao.GetUserGroupList(uid, group_list);
}

bool PostgresMgr::GetGroupMemberList(const int64_t& group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list) {
	return _dao.GetGroupMemberList(group_id, member_list);
}

bool PostgresMgr::InviteGroupMember(const int64_t& group_id, const int& inviter_uid, const int& target_uid, const std::string& reason) {
	return _dao.InviteGroupMember(group_id, inviter_uid, target_uid, reason);
}

bool PostgresMgr::ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason) {
	return _dao.ApplyJoinGroup(group_id, applicant_uid, reason);
}

bool PostgresMgr::ReviewGroupApply(const int64_t& apply_id, const int& reviewer_uid, const bool& agree, std::shared_ptr<GroupApplyInfo>& apply_info) {
	return _dao.ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info);
}

bool PostgresMgr::SaveGroupMessage(const GroupMessageInfo& msg, int64_t* out_server_msg_id, int64_t* out_group_seq, int64_t assigned_group_seq) {
	return _dao.SaveGroupMessage(msg, out_server_msg_id, out_group_seq, assigned_group_seq);
}

bool PostgresMgr::UpdateGroupMessageContent(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	return _dao.UpdateGroupMessageContent(group_id, operator_uid, msg_id, content, edited_at_ms);
}

bool PostgresMgr::RevokeGroupMessage(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
	int64_t deleted_at_ms) {
	return _dao.RevokeGroupMessage(group_id, operator_uid, msg_id, deleted_at_ms);
}

bool PostgresMgr::GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int64_t& before_seq, const int& limit,
	std::vector<std::shared_ptr<GroupMessageInfo>>& messages, bool& has_more) {
	return _dao.GetGroupHistory(group_id, before_ts, before_seq, limit, messages, has_more);
}

bool PostgresMgr::UpdateGroupAnnouncement(const int64_t& group_id, const int& operator_uid, const std::string& announcement) {
	return _dao.UpdateGroupAnnouncement(group_id, operator_uid, announcement);
}

bool PostgresMgr::UpdateGroupIcon(const int64_t& group_id, const int& operator_uid, const std::string& icon) {
	return _dao.UpdateGroupIcon(group_id, operator_uid, icon);
}

bool PostgresMgr::SetGroupAdmin(const int64_t& group_id, const int& operator_uid, const int& target_uid, const bool& is_admin, const int64_t& permission_bits) {
	return _dao.SetGroupAdmin(group_id, operator_uid, target_uid, is_admin, permission_bits);
}

bool PostgresMgr::MuteGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid, const int64_t& mute_until) {
	return _dao.MuteGroupMember(group_id, operator_uid, target_uid, mute_until);
}

bool PostgresMgr::KickGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid) {
	return _dao.KickGroupMember(group_id, operator_uid, target_uid);
}

bool PostgresMgr::QuitGroup(const int64_t& group_id, const int& uid) {
	return _dao.QuitGroup(group_id, uid);
}

bool PostgresMgr::GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info) {
	return _dao.GetGroupById(group_id, group_info);
}

bool PostgresMgr::GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role) {
	return _dao.GetUserRoleInGroup(group_id, uid, role);
}

bool PostgresMgr::IsUserInGroup(const int64_t& group_id, const int& uid) {
	return _dao.IsUserInGroup(group_id, uid);
}

bool PostgresMgr::GetPendingGroupApplyForReviewer(const int& reviewer_uid, std::vector<std::shared_ptr<GroupApplyInfo>>& applies, int limit) {
	return _dao.GetPendingGroupApplyForReviewer(reviewer_uid, applies, limit);
}

bool PostgresMgr::GetDialogMetaByOwner(const int& owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas) {
	return _dao.GetDialogMetaByOwner(owner_uid, metas);
}

bool PostgresMgr::GetPrivateDialogRuntime(const int& owner_uid, const int& peer_uid, DialogRuntimeInfo& runtime) {
	return _dao.GetPrivateDialogRuntime(owner_uid, peer_uid, runtime);
}

bool PostgresMgr::GetGroupDialogRuntime(const int& owner_uid, const int64_t& group_id, DialogRuntimeInfo& runtime) {
	return _dao.GetGroupDialogRuntime(owner_uid, group_id, runtime);
}

bool PostgresMgr::RefreshDialogsForOwner(const int& owner_uid) {
	return _dao.RefreshDialogsForOwner(owner_uid);
}

bool PostgresMgr::UpsertGroupReadState(const int& uid, const int64_t& group_id, const int64_t& read_ts) {
	return _dao.UpsertGroupReadState(uid, group_id, read_ts);
}

bool PostgresMgr::GetGroupMessageByMsgId(const int64_t& group_id, const std::string& msg_id, std::shared_ptr<GroupMessageInfo>& message) {
	return _dao.GetGroupMessageByMsgId(group_id, msg_id, message);
}

bool PostgresMgr::UpsertDialogDraft(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const std::string& draft_text) {
	return _dao.UpsertDialogDraft(owner_uid, dialog_type, peer_uid, group_id, draft_text);
}

bool PostgresMgr::UpsertDialogPinned(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const int& pinned_rank) {
	return _dao.UpsertDialogPinned(owner_uid, dialog_type, peer_uid, group_id, pinned_rank);
}

bool PostgresMgr::UpsertDialogMuteState(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const int& mute_state) {
	return _dao.UpsertDialogMuteState(owner_uid, dialog_type, peer_uid, group_id, mute_state);
}

bool PostgresMgr::GetUndeliveredPrivateMessages(const int& to_uid, const int64_t& since_read_ts, int limit,
	std::vector<std::shared_ptr<PrivateMessageInfo>>& messages) {
	return _dao.GetUndeliveredPrivateMessages(to_uid, since_read_ts, limit, messages);
}
