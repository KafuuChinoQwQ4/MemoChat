#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "Singleton.h"
#include <vector>

class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	int RegUser(const std::string& name, const std::string& email,  const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	bool AddFriendApply(const int& from, const int& to);
	bool AuthFriendApply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool ReplaceApplyTags(const int& from, const int& to, const std::vector<std::string>& tags);
	bool ReplaceFriendTags(const int& self_id, const int& friend_id, const std::vector<std::string>& tags);
	std::vector<std::string> GetApplyTags(const int& from, const int& to);
	std::vector<std::string> GetFriendTags(const int& self_id, const int& friend_id);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit=10);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);
	bool IsFriend(const int& self_id, const int& friend_id);
	bool CreateGroup(const int& owner_uid, const std::string& name, const std::string& announcement,
		const int& member_limit, const std::vector<int>& initial_members, int64_t& out_group_id);
	bool GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list);
	bool GetGroupMemberList(const int64_t& group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list);
	bool InviteGroupMember(const int64_t& group_id, const int& inviter_uid, const int& target_uid, const std::string& reason);
	bool ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason);
	bool ReviewGroupApply(const int64_t& apply_id, const int& reviewer_uid, const bool& agree, std::shared_ptr<GroupApplyInfo>& apply_info);
	bool SaveGroupMessage(const GroupMessageInfo& msg);
	bool GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int& limit,
		std::vector<std::shared_ptr<GroupMessageInfo>>& messages);
	bool UpdateGroupAnnouncement(const int64_t& group_id, const int& operator_uid, const std::string& announcement);
	bool SetGroupAdmin(const int64_t& group_id, const int& operator_uid, const int& target_uid, const bool& is_admin);
	bool MuteGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid, const int64_t& mute_until);
	bool KickGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid);
	bool QuitGroup(const int64_t& group_id, const int& uid);
	bool GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info);
	bool GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role);
	bool IsUserInGroup(const int64_t& group_id, const int& uid);
	bool GetPendingGroupApplyForReviewer(const int& reviewer_uid, std::vector<std::shared_ptr<GroupApplyInfo>>& applies, int limit = 20);
private:
	MysqlMgr();
	MysqlDao  _dao;
};
