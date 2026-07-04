#pragma once

#include "ports/IMessageRepository.hpp"

class PostgresMgr;
class MongoMgr;

class ChatMessageRepository : public IMessageRepository
{
public:
    ChatMessageRepository(PostgresMgr& pg, MongoMgr& mongo);

    bool SavePrivateMessage(const PrivateMessageInfo& msg) override;
    bool FindPrivateMessageByClientId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message) override;
    bool IsPrivateFriend(int self_id, int friend_id) override;
    bool UpsertPrivateReadState(int uid, int peer_uid, int64_t read_ts) override;
    bool UpdatePrivateMessageContent(int uid,
                                     int peer_uid,
                                     const std::string& msg_id,
                                     const std::string& content,
                                     int64_t edited_at_ms) override;
    bool RevokePrivateMessage(int uid, int peer_uid, const std::string& msg_id, int64_t deleted_at_ms) override;
    bool GetPrivateHistory(int uid,
                           int peer_uid,
                           int64_t before_ts,
                           const std::string& before_msg_id,
                           int limit,
                           std::vector<std::shared_ptr<PrivateMessageInfo>>& messages,
                           bool& has_more) override;
    bool SaveGroupMessage(const GroupMessageInfo& msg,
                          int64_t* out_server_msg_id = nullptr,
                          int64_t* out_group_seq = nullptr,
                          int64_t assigned_group_seq = 0) override;
    bool FindGroupMessageByClientId(int64_t group_id,
                                    const std::string& msg_id,
                                    std::shared_ptr<GroupMessageInfo>& message) override;
    bool GetGroupHistory(int64_t group_id,
                         int64_t before_ts,
                         int64_t before_seq,
                         int limit,
                         std::vector<std::shared_ptr<GroupMessageInfo>>& messages,
                         bool& has_more) override;
    bool UpdateGroupMessageContent(int64_t group_id,
                                   int operator_uid,
                                   const std::string& msg_id,
                                   const std::string& content,
                                   int64_t edited_at_ms) override;
    bool
    RevokeGroupMessage(int64_t group_id, int operator_uid, const std::string& msg_id, int64_t deleted_at_ms) override;
    bool UpsertGroupReadState(int uid, int64_t group_id, int64_t read_ts) override;

private:
    PostgresMgr& _pg;
    MongoMgr& _mongo;
};
