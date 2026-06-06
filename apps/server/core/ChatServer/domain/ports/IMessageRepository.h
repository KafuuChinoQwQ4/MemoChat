#pragma once

#include "data.h"

#include <memory>
#include <string>
#include <vector>

class IMessageRepository
{
public:
    virtual ~IMessageRepository() = default;

    virtual bool SavePrivateMessage(const PrivateMessageInfo& msg) = 0;
    virtual bool FindPrivateMessageByClientId(const std::string& msg_id,
                                              std::shared_ptr<PrivateMessageInfo>& message) = 0;
    virtual bool IsPrivateFriend(int self_id, int friend_id) = 0;
    virtual bool UpsertPrivateReadState(int uid, int peer_uid, int64_t read_ts) = 0;
    virtual bool UpdatePrivateMessageContent(int uid,
                                             int peer_uid,
                                             const std::string& msg_id,
                                             const std::string& content,
                                             int64_t edited_at_ms) = 0;
    virtual bool RevokePrivateMessage(int uid, int peer_uid, const std::string& msg_id, int64_t deleted_at_ms) = 0;
    virtual bool GetPrivateHistory(int uid,
                                   int peer_uid,
                                   int64_t before_ts,
                                   const std::string& before_msg_id,
                                   int limit,
                                   std::vector<std::shared_ptr<PrivateMessageInfo>>& messages,
                                   bool& has_more) = 0;
    virtual bool SaveGroupMessage(const GroupMessageInfo& msg,
                                  int64_t* out_server_msg_id = nullptr,
                                  int64_t* out_group_seq = nullptr,
                                  int64_t assigned_group_seq = 0) = 0;
    virtual bool FindGroupMessageByClientId(int64_t group_id,
                                            const std::string& msg_id,
                                            std::shared_ptr<GroupMessageInfo>& message) = 0;
    virtual bool GetGroupHistory(int64_t group_id,
                                 int64_t before_ts,
                                 int64_t before_seq,
                                 int limit,
                                 std::vector<std::shared_ptr<GroupMessageInfo>>& messages,
                                 bool& has_more) = 0;
    virtual bool UpdateGroupMessageContent(int64_t group_id,
                                           int operator_uid,
                                           const std::string& msg_id,
                                           const std::string& content,
                                           int64_t edited_at_ms) = 0;
    virtual bool
    RevokeGroupMessage(int64_t group_id, int operator_uid, const std::string& msg_id, int64_t deleted_at_ms) = 0;
    virtual bool UpsertGroupReadState(int uid, int64_t group_id, int64_t read_ts) = 0;
};
