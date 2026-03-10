#include "MongoMgr.h"

MongoMgr::MongoMgr() {
}

MongoMgr::~MongoMgr() {
}

bool MongoMgr::Enabled() const {
	return dao_.Enabled();
}

bool MongoMgr::SavePrivateMessage(const PrivateMessageInfo& msg) {
	return dao_.SavePrivateMessage(msg);
}

bool MongoMgr::GetPrivateHistory(const int& uid, const int& peer_uid, const int64_t& before_ts, const std::string& before_msg_id,
	const int& limit, std::vector<std::shared_ptr<PrivateMessageInfo>>& messages, bool& has_more) {
	return dao_.GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
}

bool MongoMgr::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message) {
	return dao_.GetPrivateMessageByMsgId(msg_id, message);
}

bool MongoMgr::UpdatePrivateMessageContent(const int& uid, const int& peer_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	return dao_.UpdatePrivateMessageContent(uid, peer_uid, msg_id, content, edited_at_ms);
}

bool MongoMgr::RevokePrivateMessage(const int& uid, const int& peer_uid, const std::string& msg_id, int64_t deleted_at_ms) {
	return dao_.RevokePrivateMessage(uid, peer_uid, msg_id, deleted_at_ms);
}

bool MongoMgr::SaveGroupMessage(const GroupMessageInfo& msg) {
	return dao_.SaveGroupMessage(msg);
}

bool MongoMgr::GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int64_t& before_seq, const int& limit,
	std::vector<std::shared_ptr<GroupMessageInfo>>& messages, bool& has_more) {
	return dao_.GetGroupHistory(group_id, before_ts, before_seq, limit, messages, has_more);
}

bool MongoMgr::GetGroupMessageByMsgId(const int64_t& group_id, const std::string& msg_id, std::shared_ptr<GroupMessageInfo>& message) {
	return dao_.GetGroupMessageByMsgId(group_id, msg_id, message);
}

bool MongoMgr::UpdateGroupMessageContent(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	return dao_.UpdateGroupMessageContent(group_id, operator_uid, msg_id, content, edited_at_ms);
}

bool MongoMgr::RevokeGroupMessage(const int64_t& group_id, const int& operator_uid, const std::string& msg_id, int64_t deleted_at_ms) {
	return dao_.RevokeGroupMessage(group_id, operator_uid, msg_id, deleted_at_ms);
}
