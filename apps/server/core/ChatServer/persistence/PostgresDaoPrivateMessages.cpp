#include "PostgresDao.hpp"
#include "PostgresDaoUtil.hpp"
#include "db/PqxxCompat.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

import memochat.chat.postgres_dao_private_messages_algorithms;

namespace postgres_dao_private_messages_modules = memochat::chat::persistence::postgres_dao_private_messages::modules;

bool PostgresDao::SavePrivateMessage(const PrivateMessageInfo& msg)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto result =
        txn.exec_params("INSERT INTO chat_private_msg(msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                        "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
                        "VALUES($1,$2,$3,$4,$5,$6,$7,NULLIF($8, '')::jsonb,$9,$10,$11) "
                        "ON CONFLICT (msg_id) DO UPDATE SET "
                        "from_uid = EXCLUDED.from_uid, to_uid = EXCLUDED.to_uid, content = EXCLUDED.content, "
                        "reply_to_server_msg_id = EXCLUDED.reply_to_server_msg_id, forward_meta_json = "
                        "EXCLUDED.forward_meta_json, "
                        "edited_at_ms = EXCLUDED.edited_at_ms, deleted_at_ms = EXCLUDED.deleted_at_ms, created_at = "
                        "EXCLUDED.created_at",
                        msg.msg_id,
                        msg.conv_uid_min,
                        msg.conv_uid_max,
                        msg.from_uid,
                        msg.to_uid,
                        msg.content,
                        msg.reply_to_server_msg_id,
                        msg.forward_meta_json,
                        msg.edited_at_ms,
                        msg.deleted_at_ms,
                        msg.created_at);
    if (!result.ok())
    {
        const auto& postgres_error = result.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetUndeliveredPrivateMessages(const int& to_uid,
                                                const int64_t& after_created_at,
                                                const std::string& after_msg_id,
                                                int limit,
                                                std::vector<std::shared_ptr<PrivateMessageInfo>>& messages)
{
    messages.clear();
    if (!postgres_dao_private_messages_modules::CanReadUndeliveredPrivateMessages(to_uid, limit))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params(
        "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
        "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
        "FROM chat_private_msg "
        "WHERE to_uid = $1 AND (created_at > $2 OR (created_at = $2 AND msg_id > $3)) AND deleted_at_ms = 0 "
        "AND created_at > COALESCE(( "
        "    SELECT read_ts FROM chat_private_read_state "
        "    WHERE uid = $1 AND peer_uid = chat_private_msg.from_uid "
        "), 0) "
        "ORDER BY created_at ASC, msg_id ASC "
        "LIMIT $4",
        to_uid,
        after_created_at,
        after_msg_id,
        limit);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    for (const auto& row : rows)
    {
        auto msg = std::make_shared<PrivateMessageInfo>();
        msg->msg_id = row["msg_id"].is_null() ? "" : row["msg_id"].c_str();
        msg->conv_uid_min = row["conv_uid_min"].as<int>();
        msg->conv_uid_max = row["conv_uid_max"].as<int>();
        msg->from_uid = row["from_uid"].as<int>();
        msg->to_uid = row["to_uid"].as<int>();
        msg->content = row["content"].is_null() ? "" : row["content"].c_str();
        msg->reply_to_server_msg_id =
            row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
        msg->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
        msg->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
        msg->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
        msg->created_at = row["created_at"].is_null() ? 0 : row["created_at"].as<int64_t>();
        messages.push_back(msg);
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetPrivateHistory(const int& uid,
                                    const int& peer_uid,
                                    const int64_t& before_ts,
                                    const std::string& before_msg_id,
                                    const int& limit,
                                    std::vector<std::shared_ptr<PrivateMessageInfo>>& messages,
                                    bool& has_more)
{
    has_more = false;
    messages.clear();
    if (!postgres_dao_private_messages_modules::CanReadPrivateHistory(uid, peer_uid, limit))
    {
        return false;
    }

    const int conv_min = postgres_dao_private_messages_modules::ConversationUidMin(uid, peer_uid);
    const int conv_max = postgres_dao_private_messages_modules::ConversationUidMax(uid, peer_uid);

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const int final_limit = postgres_dao_private_messages_modules::SelectHistoryFetchLimit(limit);
    pqxx::result rows;
    if (postgres_dao_private_messages_modules::ShouldApplyHistoryTieBreaker(before_ts, before_msg_id.empty()))
    {
        rows = txn.exec_params("SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                               "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                               "FROM chat_private_msg WHERE conv_uid_min = $1 AND conv_uid_max = $2 "
                               "AND (created_at < $3 OR (created_at = $4 AND msg_id < $5)) "
                               "ORDER BY created_at DESC, msg_id DESC LIMIT $6",
                               conv_min,
                               conv_max,
                               before_ts,
                               before_ts,
                               before_msg_id,
                               final_limit);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    else if (postgres_dao_private_messages_modules::ShouldApplyTimestampCursor(before_ts))
    {
        rows =
            txn.exec_params("SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                            "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                            "FROM chat_private_msg WHERE conv_uid_min = $1 AND conv_uid_max = $2 AND created_at < $3 "
                            "ORDER BY created_at DESC, msg_id DESC LIMIT $4",
                            conv_min,
                            conv_max,
                            before_ts,
                            final_limit);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    else
    {
        rows = txn.exec_params("SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                               "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                               "FROM chat_private_msg WHERE conv_uid_min = $1 AND conv_uid_max = $2 "
                               "ORDER BY created_at DESC, msg_id DESC LIMIT $3",
                               conv_min,
                               conv_max,
                               final_limit);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }

    for (const auto& row : rows)
    {
        auto one = std::make_shared<PrivateMessageInfo>();
        one->msg_id = row["msg_id"].c_str();
        one->conv_uid_min = row["conv_uid_min"].as<int>();
        one->conv_uid_max = row["conv_uid_max"].as<int>();
        one->from_uid = row["from_uid"].as<int>();
        one->to_uid = row["to_uid"].as<int>();
        one->content = row["content"].is_null() ? "" : row["content"].c_str();
        one->reply_to_server_msg_id =
            row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
        one->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
        one->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
        one->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
        one->created_at = row["created_at"].as<int64_t>();
        messages.push_back(one);
    }

    if (postgres_dao_private_messages_modules::HasOverflowPage(static_cast<int>(messages.size()), limit))
    {
        has_more = true;
        messages.resize(limit);
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message)
{
    message = nullptr;
    if (!postgres_dao_private_messages_modules::CanFindPrivateMessage(msg_id.empty()))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                        "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                        "FROM chat_private_msg WHERE msg_id = $1 LIMIT 1",
                        msg_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return false;
    }

    const auto& row = rows[0];
    auto one = std::make_shared<PrivateMessageInfo>();
    one->msg_id = row["msg_id"].c_str();
    one->conv_uid_min = row["conv_uid_min"].as<int>();
    one->conv_uid_max = row["conv_uid_max"].as<int>();
    one->from_uid = row["from_uid"].as<int>();
    one->to_uid = row["to_uid"].as<int>();
    one->content = row["content"].is_null() ? "" : row["content"].c_str();
    one->reply_to_server_msg_id =
        row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
    one->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
    one->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
    one->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
    one->created_at = row["created_at"].as<int64_t>();
    message = one;
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::EnqueueChatOutboxEvent(const ChatOutboxEventInfo& event)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params0("INSERT INTO chat_event_outbox(event_id, topic, partition_key, payload_json, status, "
                     "retry_count, next_retry_at, created_at, published_at, last_error) "
                     "VALUES($1,$2,$3,$4::jsonb,$5,$6,$7,$8,NULLIF($9,0),$10) "
                     "ON CONFLICT (event_id) DO NOTHING",
                     event.event_id,
                     event.topic,
                     event.partition_key,
                     event.payload_json,
                     event.status,
                     event.retry_count,
                     event.next_retry_at,
                     event.created_at > 0 ? event.created_at : NowMs(),
                     event.published_at,
                     event.last_error);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetPendingChatOutboxEvents(int limit, std::vector<ChatOutboxEventInfo>& events)
{
    events.clear();
    if (limit <= 0)
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto now_ms = NowMs();
    const auto rows =
        txn.exec_params("WITH picked AS ("
                        "    SELECT id FROM chat_event_outbox "
                        "    WHERE status = 0 AND next_retry_at <= $1 "
                        "    ORDER BY id ASC LIMIT $2 "
                        "    FOR UPDATE SKIP LOCKED"
                        ") "
                        "UPDATE chat_event_outbox AS o "
                        "SET status = 3 "
                        "FROM picked "
                        "WHERE o.id = picked.id "
                        "RETURNING o.id, o.event_id, o.topic, o.partition_key, o.payload_json::text, o.status, "
                        "o.retry_count, o.next_retry_at, o.created_at, "
                        "COALESCE(o.published_at, 0) AS published_at, COALESCE(o.last_error, '') AS last_error",
                        now_ms,
                        limit);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    for (const auto& row : rows)
    {
        ChatOutboxEventInfo one;
        one.id = row["id"].as<int64_t>();
        one.event_id = row["event_id"].c_str();
        one.topic = row["topic"].c_str();
        one.partition_key = row["partition_key"].c_str();
        one.payload_json = row["payload_json"].c_str();
        one.status = row["status"].as<int>();
        one.retry_count = row["retry_count"].as<int>();
        one.next_retry_at = row["next_retry_at"].as<int64_t>();
        one.created_at = row["created_at"].as<int64_t>();
        one.published_at = row["published_at"].as<int64_t>();
        one.last_error = row["last_error"].c_str();
        events.push_back(one);
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::MarkChatOutboxEventPublished(int64_t id, int64_t published_at_ms)
{
    if (id <= 0)
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("UPDATE chat_event_outbox SET status = 1, published_at = $2, last_error = '' WHERE id = $1",
                        id,
                        published_at_ms > 0 ? published_at_ms : NowMs());
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return rows.affected_rows() > 0;
}

bool PostgresDao::MarkChatOutboxEventRetry(int64_t id,
                                           int retry_count,
                                           int64_t next_retry_at_ms,
                                           const std::string& last_error,
                                           bool terminal_error)
{
    if (id <= 0)
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params("UPDATE chat_event_outbox SET status = $2, retry_count = $3, next_retry_at = "
                                      "$4, last_error = $5 WHERE id = $1",
                                      id,
                                      terminal_error ? 2 : 0,
                                      retry_count,
                                      next_retry_at_ms,
                                      last_error);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return rows.affected_rows() > 0;
}

bool PostgresDao::ExpediteChatOutboxEventRetry(int64_t id)
{
    if (id <= 0)
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("UPDATE chat_event_outbox SET status = 0, next_retry_at = $2 WHERE id = $1 AND status <> 1",
                        id,
                        NowMs());
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return rows.affected_rows() > 0;
}

bool PostgresDao::UpdatePrivateMessageContent(const int& uid,
                                              const int& peer_uid,
                                              const std::string& msg_id,
                                              const std::string& content,
                                              int64_t edited_at_ms)
{
    if (!postgres_dao_private_messages_modules::CanUpdatePrivateMessage(uid, peer_uid, msg_id.empty(), content.empty()))
    {
        return false;
    }
    if (postgres_dao_private_messages_modules::ShouldUseFallbackTimestamp(edited_at_ms))
    {
        edited_at_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    std::shared_ptr<PrivateMessageInfo> message;
    if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
    {
        return false;
    }
    const int conv_min = postgres_dao_private_messages_modules::ConversationUidMin(uid, peer_uid);
    const int conv_max = postgres_dao_private_messages_modules::ConversationUidMax(uid, peer_uid);
    if (!postgres_dao_private_messages_modules::IsPrivateMessageOwner(message->conv_uid_min,
                                                                      message->conv_uid_max,
                                                                      message->from_uid,
                                                                      uid,
                                                                      peer_uid))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated =
        txn.exec_params("UPDATE chat_private_msg SET content = $1, edited_at_ms = $2, deleted_at_ms = 0 "
                        "WHERE msg_id = $3 AND conv_uid_min = $4 AND conv_uid_max = $5",
                        content,
                        edited_at_ms,
                        msg_id,
                        conv_min,
                        conv_max);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return updated.affected_rows() > 0;
}

bool PostgresDao::RevokePrivateMessage(const int& uid,
                                       const int& peer_uid,
                                       const std::string& msg_id,
                                       int64_t deleted_at_ms)
{
    if (!postgres_dao_private_messages_modules::CanRequestPrivateMessageRevoke(uid, peer_uid, msg_id.empty()))
    {
        return false;
    }
    if (postgres_dao_private_messages_modules::ShouldUseFallbackTimestamp(deleted_at_ms))
    {
        deleted_at_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    std::shared_ptr<PrivateMessageInfo> message;
    if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
    {
        return false;
    }
    const int conv_min = postgres_dao_private_messages_modules::ConversationUidMin(uid, peer_uid);
    const int conv_max = postgres_dao_private_messages_modules::ConversationUidMax(uid, peer_uid);
    const long long revoke_window_ms = postgres_dao_private_messages_modules::MessageRevokeWindowMs();
    const bool owner_matches = postgres_dao_private_messages_modules::IsPrivateMessageOwner(message->conv_uid_min,
                                                                                            message->conv_uid_max,
                                                                                            message->from_uid,
                                                                                            uid,
                                                                                            peer_uid);
    if (!postgres_dao_private_messages_modules::CanRevokePrivateMessage(owner_matches,
                                                                        message->deleted_at_ms,
                                                                        message->created_at,
                                                                        deleted_at_ms,
                                                                        revoke_window_ms))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated =
        txn.exec_params("UPDATE chat_private_msg SET content = '[消息已撤回]', deleted_at_ms = $1, edited_at_ms = 0 "
                        "WHERE msg_id = $2 AND conv_uid_min = $3 AND conv_uid_max = $4 AND from_uid = $5 "
                        "AND deleted_at_ms = 0 AND created_at >= $6",
                        deleted_at_ms,
                        msg_id,
                        conv_min,
                        conv_max,
                        uid,
                        postgres_dao_private_messages_modules::RevokeWindowStart(deleted_at_ms, revoke_window_ms));
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return updated.affected_rows() > 0;
}

bool PostgresDao::UpsertPrivateReadState(const int& uid, const int& peer_uid, const int64_t& read_ts)
{
    if (!postgres_dao_private_messages_modules::CanUpsertPrivateReadState(uid, peer_uid))
    {
        return false;
    }
    int64_t normalized_read_ts = read_ts;
    if (postgres_dao_private_messages_modules::ShouldUseFallbackTimestamp(normalized_read_ts))
    {
        normalized_read_ts = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto result =
        txn.exec_params("INSERT INTO chat_private_read_state(uid, peer_uid, read_ts) VALUES($1, $2, $3) "
                        "ON CONFLICT (uid, peer_uid) DO UPDATE SET "
                        "read_ts = GREATEST(chat_private_read_state.read_ts, EXCLUDED.read_ts), "
                        "updated_at = CURRENT_TIMESTAMP",
                        uid,
                        peer_uid,
                        normalized_read_ts);
    if (!result.ok())
    {
        const auto& postgres_error = result.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}
