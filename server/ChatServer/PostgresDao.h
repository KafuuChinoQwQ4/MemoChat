#pragma once
#include "data.h"

#include <memory>

class PostgresPool;

class PostgresDao
{
public:
	PostgresDao();
	~PostgresDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
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
	bool GetUidByUserId(const std::string& user_id, int& uid);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit );
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);
	bool SavePrivateMessage(const PrivateMessageInfo& msg);
	bool EnqueueChatOutboxEvent(const ChatOutboxEventInfo& event);
	bool GetPendingChatOutboxEvents(int limit, std::vector<ChatOutboxEventInfo>& events);
	bool MarkChatOutboxEventPublished(int64_t id, int64_t published_at_ms);
	bool MarkChatOutboxEventRetry(int64_t id, int retry_count, int64_t next_retry_at_ms, const std::string& last_error, bool terminal_error);
	bool ExpediteChatOutboxEventRetry(int64_t id);
	bool GetPrivateHistory(const int& uid, const int& peer_uid, const int64_t& before_ts, const std::string& before_msg_id, const int& limit,
		std::vector<std::shared_ptr<PrivateMessageInfo>>& messages, bool& has_more);
	bool GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message);
	bool UpdatePrivateMessageContent(const int& uid, const int& peer_uid, const std::string& msg_id,
		const std::string& content, int64_t edited_at_ms = 0);
	bool RevokePrivateMessage(const int& uid, const int& peer_uid, const std::string& msg_id,
		int64_t deleted_at_ms = 0);
	bool UpsertPrivateReadState(const int& uid, const int& peer_uid, const int64_t& read_ts);
	bool IsFriend(const int& self_id, const int& friend_id);

	bool CreateGroup(const int& owner_uid, const std::string& name, const std::string& announcement,
		const int& member_limit, const std::vector<int>& initial_members, int64_t& out_group_id, std::string& out_group_code);
	bool GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id);
	bool GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list);
	bool GetGroupMemberList(const int64_t& group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list);
	bool InviteGroupMember(const int64_t& group_id, const int& inviter_uid, const int& target_uid, const std::string& reason);
	bool ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason);
	bool ReviewGroupApply(const int64_t& apply_id, const int& reviewer_uid, const bool& agree, std::shared_ptr<GroupApplyInfo>& apply_info);
	bool SaveGroupMessage(const GroupMessageInfo& msg, int64_t* out_server_msg_id = nullptr, int64_t* out_group_seq = nullptr, int64_t assigned_group_seq = 0);
	bool UpdateGroupMessageContent(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
		const std::string& content, int64_t edited_at_ms = 0);
	bool RevokeGroupMessage(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
		int64_t deleted_at_ms = 0);
	bool GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int64_t& before_seq, const int& limit,
		std::vector<std::shared_ptr<GroupMessageInfo>>& messages, bool& has_more);
	bool UpdateGroupAnnouncement(const int64_t& group_id, const int& operator_uid, const std::string& announcement);
	bool UpdateGroupIcon(const int64_t& group_id, const int& operator_uid, const std::string& icon);
	bool SetGroupAdmin(const int64_t& group_id, const int& operator_uid, const int& target_uid, const bool& is_admin, const int64_t& permission_bits = 0);
	bool MuteGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid, const int64_t& mute_until);
	bool KickGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid);
	bool QuitGroup(const int64_t& group_id, const int& uid);
	bool GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info);
	bool GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role);
	bool IsUserInGroup(const int64_t& group_id, const int& uid);
	bool GetPendingGroupApplyForReviewer(const int& reviewer_uid, std::vector<std::shared_ptr<GroupApplyInfo>>& applies, int limit);
	bool GetDialogMetaByOwner(const int& owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas);
	bool GetPrivateDialogRuntime(const int& owner_uid, const int& peer_uid, DialogRuntimeInfo& runtime);
	bool GetGroupDialogRuntime(const int& owner_uid, const int64_t& group_id, DialogRuntimeInfo& runtime);
	bool RefreshDialogsForOwner(const int& owner_uid);
	bool UpsertGroupReadState(const int& uid, const int64_t& group_id, const int64_t& read_ts);
	bool GetGroupMessageByMsgId(const int64_t& group_id, const std::string& msg_id, std::shared_ptr<GroupMessageInfo>& message);
	bool UpsertDialogDraft(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
		const int64_t& group_id, const std::string& draft_text);
	bool UpsertDialogPinned(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
		const int64_t& group_id, const int& pinned_rank);
	bool UpsertDialogMuteState(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
		const int64_t& group_id, const int& mute_state);
	bool GetUndeliveredPrivateMessages(const int& to_uid, const int64_t& since_read_ts, int limit,
		std::vector<std::shared_ptr<PrivateMessageInfo>>& messages);
private:
	void WarmupRelationBootstrapQueries();
	bool EnsureGroupCodeSchemaAndBackfill();
	bool EnsureDialogMetaSchema();
	bool EnsurePrivateReadStateSchema();
	bool EnsureGroupReadStateSchema();
	bool EnsureGroupMessageOrderSchema();
	bool EnsureGroupPermissionSchemaAndBackfill();
	bool EnsureChatEventOutboxSchema();
	bool EnsureChatMessageIdempotencySchema();
	bool GetGroupPermissionBits(const int64_t& group_id, const int& uid, int64_t& out_bits);
	bool HasGroupPermission(const int64_t& group_id, const int& uid, int64_t required_bits);
	std::string GenerateGroupCode();
	std::unique_ptr<PostgresPool> pool_;
	bool use_postgres_ = false;
	std::string postgres_connection_string_;
};
