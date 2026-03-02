#include "MysqlMgr.h"


MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _dao.RegUser(name, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

MysqlMgr::MysqlMgr() {
}

bool MysqlMgr::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(name, pwd, userInfo);
}

bool MysqlMgr::AddFriendApply(const int& from, const int& to)
{
	return _dao.AddFriendApply(from, to);
}

bool MysqlMgr::AuthFriendApply(const int& from, const int& to) {
	return _dao.AuthFriendApply(from, to);
}

bool MysqlMgr::AddFriend(const int& from, const int& to, std::string back_name) {
	return _dao.AddFriend(from, to, back_name);
}

bool MysqlMgr::ReplaceApplyTags(const int& from, const int& to, const std::vector<std::string>& tags) {
	return _dao.ReplaceApplyTags(from, to, tags);
}

bool MysqlMgr::ReplaceFriendTags(const int& self_id, const int& friend_id, const std::vector<std::string>& tags) {
	return _dao.ReplaceFriendTags(self_id, friend_id, tags);
}

std::vector<std::string> MysqlMgr::GetApplyTags(const int& from, const int& to) {
	return _dao.GetApplyTags(from, to);
}

std::vector<std::string> MysqlMgr::GetFriendTags(const int& self_id, const int& friend_id) {
	return _dao.GetFriendTags(self_id, friend_id);
}
std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
{
	return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(std::string name)
{
	return _dao.GetUser(name);
}

bool MysqlMgr::GetApplyList(int touid, 
	std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {

	return _dao.GetApplyList(touid, applyList, begin, limit);
}

bool MysqlMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info) {
	return _dao.GetFriendList(self_id, user_info);
}

bool MysqlMgr::IsFriend(const int& self_id, const int& friend_id) {
	return _dao.IsFriend(self_id, friend_id);
}

bool MysqlMgr::CreateGroup(const int& owner_uid, const std::string& name, const std::string& announcement,
	const int& member_limit, const std::vector<int>& initial_members, int64_t& out_group_id) {
	return _dao.CreateGroup(owner_uid, name, announcement, member_limit, initial_members, out_group_id);
}

bool MysqlMgr::GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list) {
	return _dao.GetUserGroupList(uid, group_list);
}

bool MysqlMgr::GetGroupMemberList(const int64_t& group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list) {
	return _dao.GetGroupMemberList(group_id, member_list);
}

bool MysqlMgr::InviteGroupMember(const int64_t& group_id, const int& inviter_uid, const int& target_uid, const std::string& reason) {
	return _dao.InviteGroupMember(group_id, inviter_uid, target_uid, reason);
}

bool MysqlMgr::ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason) {
	return _dao.ApplyJoinGroup(group_id, applicant_uid, reason);
}

bool MysqlMgr::ReviewGroupApply(const int64_t& apply_id, const int& reviewer_uid, const bool& agree, std::shared_ptr<GroupApplyInfo>& apply_info) {
	return _dao.ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info);
}

bool MysqlMgr::SaveGroupMessage(const GroupMessageInfo& msg) {
	return _dao.SaveGroupMessage(msg);
}

bool MysqlMgr::GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int& limit,
	std::vector<std::shared_ptr<GroupMessageInfo>>& messages) {
	return _dao.GetGroupHistory(group_id, before_ts, limit, messages);
}

bool MysqlMgr::UpdateGroupAnnouncement(const int64_t& group_id, const int& operator_uid, const std::string& announcement) {
	return _dao.UpdateGroupAnnouncement(group_id, operator_uid, announcement);
}

bool MysqlMgr::SetGroupAdmin(const int64_t& group_id, const int& operator_uid, const int& target_uid, const bool& is_admin) {
	return _dao.SetGroupAdmin(group_id, operator_uid, target_uid, is_admin);
}

bool MysqlMgr::MuteGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid, const int64_t& mute_until) {
	return _dao.MuteGroupMember(group_id, operator_uid, target_uid, mute_until);
}

bool MysqlMgr::KickGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid) {
	return _dao.KickGroupMember(group_id, operator_uid, target_uid);
}

bool MysqlMgr::QuitGroup(const int64_t& group_id, const int& uid) {
	return _dao.QuitGroup(group_id, uid);
}

bool MysqlMgr::GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info) {
	return _dao.GetGroupById(group_id, group_info);
}

bool MysqlMgr::GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role) {
	return _dao.GetUserRoleInGroup(group_id, uid, role);
}

bool MysqlMgr::IsUserInGroup(const int64_t& group_id, const int& uid) {
	return _dao.IsUserInGroup(group_id, uid);
}

bool MysqlMgr::GetPendingGroupApplyForReviewer(const int& reviewer_uid, std::vector<std::shared_ptr<GroupApplyInfo>>& applies, int limit) {
	return _dao.GetPendingGroupApplyForReviewer(reviewer_uid, applies, limit);
}
