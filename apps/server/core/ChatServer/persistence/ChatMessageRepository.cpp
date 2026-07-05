#include "ChatMessageRepository.hpp"

#include "MongoMgr.hpp"
#include "PostgresMgr.hpp"

#include <iostream>

import memochat.chat.message_repository_algorithms;

namespace
{
namespace message_repository_modules = memochat::chat::persistence::message_repository::modules;

template <typename PrimaryWriteFn, typename MongoWriteFn>
bool WritePrimaryAndMirrorToMongo(MongoMgr& mongo,
                                  const char* operation,
                                  const std::string& msg_id,
                                  PrimaryWriteFn primary_write,
                                  MongoWriteFn mongo_write)
{
    if (message_repository_modules::ShouldAbortAfterPrimaryWrite(primary_write()))
    {
        return false;
    }

    const bool mongo_enabled = mongo.Enabled();
    if (message_repository_modules::ShouldAttemptMongo(mongo_enabled))
    {
        const bool mongo_write_success = mongo_write();
        if (message_repository_modules::ShouldLogMongoWriteFailure(mongo_enabled, mongo_write_success))
        {
            std::cerr << "[MongoMgr] " << operation << " failed for msg_id=" << msg_id << std::endl;
        }
    }
    return true;
}

template <typename MongoReadFn, typename PostgresReadFn, typename ResultEmptyFn>
bool ReadMongoThenPostgres(MongoMgr& mongo,
                           MongoReadFn mongo_read,
                           PostgresReadFn postgres_read,
                           ResultEmptyFn result_empty)
{
    bool mongo_success = false;
    if (message_repository_modules::ShouldAttemptMongo(mongo.Enabled()))
    {
        mongo_success = mongo_read();
    }

    bool postgres_success = false;
    if (message_repository_modules::ShouldFallbackToPostgres(mongo_success, result_empty()))
    {
        postgres_success = postgres_read();
    }
    return message_repository_modules::MergeReadSuccess(mongo_success, postgres_success);
}
} // namespace

ChatMessageRepository::ChatMessageRepository()
    : ChatMessageRepository(*PostgresMgr::GetInstance(), *MongoMgr::GetInstance())
{
}

ChatMessageRepository::ChatMessageRepository(PostgresMgr& pg, MongoMgr& mongo)
    : _pg(pg)
    , _mongo(mongo)
{
}

bool ChatMessageRepository::SavePrivateMessage(const PrivateMessageInfo& msg)
{
    return WritePrimaryAndMirrorToMongo(
        _mongo,
        "SavePrivateMessage dual-write",
        msg.msg_id,
        [this, &msg]()
        {
            return _pg.SavePrivateMessage(msg);
        },
        [this, &msg]()
        {
            return _mongo.SavePrivateMessage(msg);
        });
}

bool ChatMessageRepository::FindPrivateMessageByClientId(const std::string& msg_id,
                                                         std::shared_ptr<PrivateMessageInfo>& message)
{
    return ReadMongoThenPostgres(
        _mongo,
        [this, &msg_id, &message]()
        {
            return message_repository_modules::HasLoadedMessage(_mongo.GetPrivateMessageByMsgId(msg_id, message),
                                                                message != nullptr);
        },
        [this, &msg_id, &message]()
        {
            return message_repository_modules::HasLoadedMessage(_pg.GetPrivateMessageByMsgId(msg_id, message),
                                                                message != nullptr);
        },
        [&message]()
        {
            return message == nullptr;
        });
}

bool ChatMessageRepository::IsPrivateFriend(int self_id, int friend_id)
{
    return _pg.IsFriend(self_id, friend_id);
}

bool ChatMessageRepository::UpsertPrivateReadState(int uid, int peer_uid, int64_t read_ts)
{
    return _pg.UpsertPrivateReadState(uid, peer_uid, read_ts);
}

bool ChatMessageRepository::UpdatePrivateMessageContent(int uid,
                                                        int peer_uid,
                                                        const std::string& msg_id,
                                                        const std::string& content,
                                                        int64_t edited_at_ms)
{
    return WritePrimaryAndMirrorToMongo(
        _mongo,
        "UpdatePrivateMessageContent sync",
        msg_id,
        [this, uid, peer_uid, &msg_id, &content, edited_at_ms]()
        {
            return _pg.UpdatePrivateMessageContent(uid, peer_uid, msg_id, content, edited_at_ms);
        },
        [this, uid, peer_uid, &msg_id, &content, edited_at_ms]()
        {
            return _mongo.UpdatePrivateMessageContent(uid, peer_uid, msg_id, content, edited_at_ms);
        });
}

bool ChatMessageRepository::RevokePrivateMessage(int uid,
                                                 int peer_uid,
                                                 const std::string& msg_id,
                                                 int64_t deleted_at_ms)
{
    return WritePrimaryAndMirrorToMongo(
        _mongo,
        "RevokePrivateMessage sync",
        msg_id,
        [this, uid, peer_uid, &msg_id, deleted_at_ms]()
        {
            return _pg.RevokePrivateMessage(uid, peer_uid, msg_id, deleted_at_ms);
        },
        [this, uid, peer_uid, &msg_id, deleted_at_ms]()
        {
            return _mongo.RevokePrivateMessage(uid, peer_uid, msg_id, deleted_at_ms);
        });
}

bool ChatMessageRepository::GetPrivateHistory(int uid,
                                              int peer_uid,
                                              int64_t before_ts,
                                              const std::string& before_msg_id,
                                              int limit,
                                              std::vector<std::shared_ptr<PrivateMessageInfo>>& messages,
                                              bool& has_more)
{
    return ReadMongoThenPostgres(
        _mongo,
        [this, uid, peer_uid, before_ts, &before_msg_id, limit, &messages, &has_more]()
        {
            return _mongo.GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
        },
        [this, uid, peer_uid, before_ts, &before_msg_id, limit, &messages, &has_more]()
        {
            return _pg.GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
        },
        [&messages]()
        {
            return messages.empty();
        });
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
    return WritePrimaryAndMirrorToMongo(
        _mongo,
        "SaveGroupMessage dual-write",
        msg.msg_id,
        [this, &msg, server_msg_id_out, group_seq_out, assigned_group_seq]()
        {
            return _pg.SaveGroupMessage(msg, server_msg_id_out, group_seq_out, assigned_group_seq);
        },
        [this, &msg, server_msg_id_out, group_seq_out]()
        {
            GroupMessageInfo persisted_msg = msg;
            persisted_msg.server_msg_id = *server_msg_id_out;
            persisted_msg.group_seq = *group_seq_out;
            return _mongo.SaveGroupMessage(persisted_msg);
        });
}

bool ChatMessageRepository::FindGroupMessageByClientId(int64_t group_id,
                                                       const std::string& msg_id,
                                                       std::shared_ptr<GroupMessageInfo>& message)
{
    return ReadMongoThenPostgres(
        _mongo,
        [this, group_id, &msg_id, &message]()
        {
            return message_repository_modules::HasLoadedMessage(
                _mongo.GetGroupMessageByMsgId(group_id, msg_id, message),
                message != nullptr);
        },
        [this, group_id, &msg_id, &message]()
        {
            return message_repository_modules::HasLoadedMessage(_pg.GetGroupMessageByMsgId(group_id, msg_id, message),
                                                                message != nullptr);
        },
        [&message]()
        {
            return message == nullptr;
        });
}

bool ChatMessageRepository::GetGroupHistory(int64_t group_id,
                                            int64_t before_ts,
                                            int64_t before_seq,
                                            int limit,
                                            std::vector<std::shared_ptr<GroupMessageInfo>>& messages,
                                            bool& has_more)
{
    return ReadMongoThenPostgres(
        _mongo,
        [this, group_id, before_ts, before_seq, limit, &messages, &has_more]()
        {
            return _mongo.GetGroupHistory(group_id, before_ts, before_seq, limit, messages, has_more);
        },
        [this, group_id, before_ts, before_seq, limit, &messages, &has_more]()
        {
            return _pg.GetGroupHistory(group_id, before_ts, before_seq, limit, messages, has_more);
        },
        [&messages]()
        {
            return messages.empty();
        });
}

bool ChatMessageRepository::UpdateGroupMessageContent(int64_t group_id,
                                                      int operator_uid,
                                                      const std::string& msg_id,
                                                      const std::string& content,
                                                      int64_t edited_at_ms)
{
    return WritePrimaryAndMirrorToMongo(
        _mongo,
        "UpdateGroupMessageContent sync",
        msg_id,
        [this, group_id, operator_uid, &msg_id, &content, edited_at_ms]()
        {
            return _pg.UpdateGroupMessageContent(group_id, operator_uid, msg_id, content, edited_at_ms);
        },
        [this, group_id, operator_uid, &msg_id, &content, edited_at_ms]()
        {
            return _mongo.UpdateGroupMessageContent(group_id, operator_uid, msg_id, content, edited_at_ms);
        });
}

bool ChatMessageRepository::RevokeGroupMessage(int64_t group_id,
                                               int operator_uid,
                                               const std::string& msg_id,
                                               int64_t deleted_at_ms)
{
    return WritePrimaryAndMirrorToMongo(
        _mongo,
        "RevokeGroupMessage sync",
        msg_id,
        [this, group_id, operator_uid, &msg_id, deleted_at_ms]()
        {
            return _pg.RevokeGroupMessage(group_id, operator_uid, msg_id, deleted_at_ms);
        },
        [this, group_id, operator_uid, &msg_id, deleted_at_ms]()
        {
            return _mongo.RevokeGroupMessage(group_id, operator_uid, msg_id, deleted_at_ms);
        });
}

bool ChatMessageRepository::UpsertGroupReadState(int uid, int64_t group_id, int64_t read_ts)
{
    return _pg.UpsertGroupReadState(uid, group_id, read_ts);
}
