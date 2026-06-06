#include "ChatMessageRepository.h"

#include "MongoMgr.h"
#include "PostgresMgr.h"

#include <iostream>

bool ChatMessageRepository::SavePrivateMessage(const PrivateMessageInfo& msg)
{
    if (!PostgresMgr::GetInstance()->SavePrivateMessage(msg))
    {
        return false;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SavePrivateMessage(msg))
    {
        std::cerr << "[MongoMgr] SavePrivateMessage dual-write failed for msg_id=" << msg.msg_id << std::endl;
    }
    return true;
}

bool ChatMessageRepository::FindPrivateMessageByClientId(const std::string& msg_id,
                                                         std::shared_ptr<PrivateMessageInfo>& message)
{
    bool mongo_success = false;
    if (MongoMgr::GetInstance()->Enabled())
    {
        mongo_success = MongoMgr::GetInstance()->GetPrivateMessageByMsgId(msg_id, message) && message;
    }
    if (mongo_success)
    {
        return true;
    }
    return PostgresMgr::GetInstance()->GetPrivateMessageByMsgId(msg_id, message) && message;
}

bool ChatMessageRepository::IsPrivateFriend(int self_id, int friend_id)
{
    return PostgresMgr::GetInstance()->IsFriend(self_id, friend_id);
}

bool ChatMessageRepository::UpsertPrivateReadState(int uid, int peer_uid, int64_t read_ts)
{
    return PostgresMgr::GetInstance()->UpsertPrivateReadState(uid, peer_uid, read_ts);
}

bool ChatMessageRepository::UpdatePrivateMessageContent(int uid,
                                                        int peer_uid,
                                                        const std::string& msg_id,
                                                        const std::string& content,
                                                        int64_t edited_at_ms)
{
    if (!PostgresMgr::GetInstance()->UpdatePrivateMessageContent(uid, peer_uid, msg_id, content, edited_at_ms))
    {
        return false;
    }
    if (MongoMgr::GetInstance()->Enabled() &&
        !MongoMgr::GetInstance()->UpdatePrivateMessageContent(uid, peer_uid, msg_id, content, edited_at_ms))
    {
        std::cerr << "[MongoMgr] UpdatePrivateMessageContent sync failed for msg_id=" << msg_id << std::endl;
    }
    return true;
}

bool ChatMessageRepository::RevokePrivateMessage(int uid,
                                                 int peer_uid,
                                                 const std::string& msg_id,
                                                 int64_t deleted_at_ms)
{
    if (!PostgresMgr::GetInstance()->RevokePrivateMessage(uid, peer_uid, msg_id, deleted_at_ms))
    {
        return false;
    }
    if (MongoMgr::GetInstance()->Enabled() &&
        !MongoMgr::GetInstance()->RevokePrivateMessage(uid, peer_uid, msg_id, deleted_at_ms))
    {
        std::cerr << "[MongoMgr] RevokePrivateMessage sync failed for msg_id=" << msg_id << std::endl;
    }
    return true;
}

bool ChatMessageRepository::GetPrivateHistory(int uid,
                                              int peer_uid,
                                              int64_t before_ts,
                                              const std::string& before_msg_id,
                                              int limit,
                                              std::vector<std::shared_ptr<PrivateMessageInfo>>& messages,
                                              bool& has_more)
{
    bool mongo_success = false;
    bool pg_success = false;
    if (MongoMgr::GetInstance()->Enabled())
    {
        mongo_success = MongoMgr::GetInstance()
                            ->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
    }
    if (!mongo_success || messages.empty())
    {
        pg_success = PostgresMgr::GetInstance()
                         ->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
    }
    return mongo_success || pg_success;
}

bool ChatMessageRepository::SaveGroupMessage(const GroupMessageInfo& msg,
                                             int64_t* out_server_msg_id,
                                             int64_t* out_group_seq,
                                             int64_t assigned_group_seq)
{
    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    int64_t* server_msg_id_out = out_server_msg_id ? out_server_msg_id : &server_msg_id;
    int64_t* group_seq_out = out_group_seq ? out_group_seq : &group_seq;
    if (!PostgresMgr::GetInstance()->SaveGroupMessage(msg, server_msg_id_out, group_seq_out, assigned_group_seq))
    {
        return false;
    }

    GroupMessageInfo persisted_msg = msg;
    persisted_msg.server_msg_id = *server_msg_id_out;
    persisted_msg.group_seq = *group_seq_out;
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SaveGroupMessage(persisted_msg))
    {
        std::cerr << "[MongoMgr] SaveGroupMessage dual-write failed for msg_id=" << msg.msg_id << std::endl;
    }
    return true;
}

bool ChatMessageRepository::FindGroupMessageByClientId(int64_t group_id,
                                                       const std::string& msg_id,
                                                       std::shared_ptr<GroupMessageInfo>& message)
{
    bool mongo_success = false;
    if (MongoMgr::GetInstance()->Enabled())
    {
        mongo_success = MongoMgr::GetInstance()->GetGroupMessageByMsgId(group_id, msg_id, message) && message;
    }
    if (mongo_success)
    {
        return true;
    }
    return PostgresMgr::GetInstance()->GetGroupMessageByMsgId(group_id, msg_id, message) && message;
}

bool ChatMessageRepository::GetGroupHistory(int64_t group_id,
                                            int64_t before_ts,
                                            int64_t before_seq,
                                            int limit,
                                            std::vector<std::shared_ptr<GroupMessageInfo>>& messages,
                                            bool& has_more)
{
    bool mongo_success = false;
    bool pg_success = false;
    if (MongoMgr::GetInstance()->Enabled())
    {
        mongo_success =
            MongoMgr::GetInstance()->GetGroupHistory(group_id, before_ts, before_seq, limit, messages, has_more);
    }
    if (!mongo_success || messages.empty())
    {
        pg_success =
            PostgresMgr::GetInstance()->GetGroupHistory(group_id, before_ts, before_seq, limit, messages, has_more);
    }
    return mongo_success || pg_success;
}

bool ChatMessageRepository::UpdateGroupMessageContent(int64_t group_id,
                                                      int operator_uid,
                                                      const std::string& msg_id,
                                                      const std::string& content,
                                                      int64_t edited_at_ms)
{
    if (!PostgresMgr::GetInstance()->UpdateGroupMessageContent(group_id, operator_uid, msg_id, content, edited_at_ms))
    {
        return false;
    }
    if (MongoMgr::GetInstance()->Enabled() &&
        !MongoMgr::GetInstance()->UpdateGroupMessageContent(group_id, operator_uid, msg_id, content, edited_at_ms))
    {
        std::cerr << "[MongoMgr] UpdateGroupMessageContent sync failed for msg_id=" << msg_id << std::endl;
    }
    return true;
}

bool ChatMessageRepository::RevokeGroupMessage(int64_t group_id,
                                               int operator_uid,
                                               const std::string& msg_id,
                                               int64_t deleted_at_ms)
{
    if (!PostgresMgr::GetInstance()->RevokeGroupMessage(group_id, operator_uid, msg_id, deleted_at_ms))
    {
        return false;
    }
    if (MongoMgr::GetInstance()->Enabled() &&
        !MongoMgr::GetInstance()->RevokeGroupMessage(group_id, operator_uid, msg_id, deleted_at_ms))
    {
        std::cerr << "[MongoMgr] RevokeGroupMessage sync failed for msg_id=" << msg_id << std::endl;
    }
    return true;
}

bool ChatMessageRepository::UpsertGroupReadState(int uid, int64_t group_id, int64_t read_ts)
{
    return PostgresMgr::GetInstance()->UpsertGroupReadState(uid, group_id, read_ts);
}
