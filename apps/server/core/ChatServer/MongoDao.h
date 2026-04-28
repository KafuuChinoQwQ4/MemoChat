#pragma once

#include "data.h"

#include <memory>
#include <string>
#include <vector>

#include <mongocxx/pool.hpp>

class MongoDao {
public:
	MongoDao();
	~MongoDao();

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
	bool Init();
	bool EnsureIndexes();
	std::string BuildPrivateConvKey(int uid_a, int uid_b) const;

	bool enabled_ = false;
	bool init_ok_ = false;
	std::string uri_;
	std::string database_name_;
	std::string private_collection_name_;
	std::string group_collection_name_;
	std::unique_ptr<mongocxx::pool> pool_;
};
