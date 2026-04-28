#pragma once

#include "MongoDao.h"
#include "Singleton.h"

class MongoMgr : public Singleton<MongoMgr>
{
	friend class Singleton<MongoMgr>;
public:
	~MongoMgr();

	bool Enabled() const;

	bool SavePrivateMessage(const PrivateMessageInfo& msg);
	bool GetPrivateHistory(const int& uid, const int& peer_uid, const int64_t& before_ts, const std::string& before_msg_id,
		const int& limit, std::vector<std::shared_ptr<PrivateMessageInfo>>& messages, bool& has_more);
	bool GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message);
	bool UpdatePrivateMessageContent(const int& uid, const int& peer_uid, const std::string& msg_id,
		const std::string& content, int64_t edited_at_ms = 0);
	bool RevokePrivateMessage(const int& uid, const int& peer_uid, const std::string& msg_id, int64_t deleted_at_ms = 0);

	bool SaveGroupMessage(const GroupMessageInfo& msg);
	bool GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int64_t& before_seq, const int& limit,
		std::vector<std::shared_ptr<GroupMessageInfo>>& messages, bool& has_more);
	bool GetGroupMessageByMsgId(const int64_t& group_id, const std::string& msg_id, std::shared_ptr<GroupMessageInfo>& message);
	bool UpdateGroupMessageContent(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
		const std::string& content, int64_t edited_at_ms = 0);
	bool RevokeGroupMessage(const int64_t& group_id, const int& operator_uid, const std::string& msg_id, int64_t deleted_at_ms = 0);

private:
	MongoMgr();
	MongoDao dao_;
};
