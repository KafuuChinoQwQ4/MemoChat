#include "PostgresDao.hpp"
#include "db/PqxxCompat.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

import memochat.chat.postgres_dao_group_messages_algorithms;

namespace postgres_dao_group_messages_modules = memochat::chat::persistence::postgres_dao_group_messages::modules;

bool PostgresDao::SaveGroupMessage(const GroupMessageInfo& msg,
                                   int64_t* out_server_msg_id,
                                   int64_t* out_group_seq,
                                   int64_t assigned_group_seq)
{
    if (!postgres_dao_group_messages_modules::CanSaveGroupMessage(msg.msg_id.empty(), msg.group_id, msg.from_uid))
    {
        return false;
    }
    if (out_server_msg_id)
    {
        *out_server_msg_id = 0;
    }
    if (out_group_seq)
    {
        *out_group_seq = 0;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto group_rows =
        txn.exec_params("SELECT group_id FROM chat_group WHERE group_id = $1 AND status = 1 LIMIT 1 FOR UPDATE",
                        msg.group_id);
    if (!group_rows.ok())
    {
        const auto& postgres_error = group_rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (group_rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return false;
    }

    int64_t next_group_seq = assigned_group_seq;
    if (postgres_dao_group_messages_modules::ShouldLoadNextGroupSeq(next_group_seq))
    {
        const auto seq_rows = txn.exec_params(
            "SELECT COALESCE(MAX(group_seq), 0) + 1 AS next_seq FROM chat_group_msg WHERE group_id = $1",
            msg.group_id);
        if (!seq_rows.ok())
        {
            const auto& postgres_error = seq_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        next_group_seq = seq_rows.empty() ? 1 : seq_rows[0]["next_seq"].as<int64_t>();
        next_group_seq = postgres_dao_group_messages_modules::NormalizeNextGroupSeq(next_group_seq);
    }

    const auto insert_rows = txn.exec_params(
        "INSERT INTO chat_group_msg(msg_id, group_id, group_seq, from_uid, msg_type, content, "
        "mentions_json, reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
        "VALUES($1,$2,$3,$4,$5,$6,COALESCE(NULLIF($7, ''), '[]')::jsonb,$8,NULLIF($9, '')::jsonb,$10,$11,$12) "
        "ON CONFLICT (group_id, msg_id) DO UPDATE SET "
        "from_uid = EXCLUDED.from_uid, msg_type = EXCLUDED.msg_type, content = EXCLUDED.content, "
        "mentions_json = EXCLUDED.mentions_json, reply_to_server_msg_id = EXCLUDED.reply_to_server_msg_id, "
        "forward_meta_json = EXCLUDED.forward_meta_json, edited_at_ms = EXCLUDED.edited_at_ms, "
        "deleted_at_ms = EXCLUDED.deleted_at_ms, created_at = EXCLUDED.created_at "
        "RETURNING server_msg_id, group_seq",
        msg.msg_id,
        msg.group_id,
        next_group_seq,
        msg.from_uid,
        msg.msg_type,
        msg.content,
        msg.mentions_json,
        msg.reply_to_server_msg_id,
        msg.forward_meta_json,
        msg.edited_at_ms,
        msg.deleted_at_ms,
        msg.created_at);
    if (!insert_rows.ok())
    {
        const auto& postgres_error = insert_rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (insert_rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return false;
    }

    const auto server_msg_id = insert_rows[0]["server_msg_id"].as<int64_t>();
    next_group_seq = insert_rows[0]["group_seq"].as<int64_t>();
    if (postgres_dao_group_messages_modules::ShouldWriteGroupMessageExt(msg.file_name.empty(),
                                                                        msg.mime.empty(),
                                                                        msg.size))
    {
        txn.exec_params0("INSERT INTO chat_group_msg_ext(msg_id, file_name, mime, size) VALUES($1,$2,$3,$4) "
                         "ON CONFLICT (msg_id) DO UPDATE SET file_name = EXCLUDED.file_name, mime = "
                         "EXCLUDED.mime, size = EXCLUDED.size",
                         msg.msg_id,
                         msg.file_name,
                         msg.mime,
                         msg.size);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }

    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (out_server_msg_id)
    {
        *out_server_msg_id = server_msg_id;
    }
    if (out_group_seq)
    {
        *out_group_seq = next_group_seq;
    }
    return true;
}

bool PostgresDao::UpdateGroupMessageContent(const int64_t& group_id,
                                            const int& operator_uid,
                                            const std::string& msg_id,
                                            const std::string& content,
                                            int64_t edited_at_ms)
{
    if (!postgres_dao_group_messages_modules::CanUpdateGroupMessage(group_id,
                                                                    operator_uid,
                                                                    msg_id.empty(),
                                                                    content.empty()))
    {
        return false;
    }
    if (postgres_dao_group_messages_modules::ShouldUseFallbackTimestamp(edited_at_ms))
    {
        edited_at_ms = static_cast<int64_t>(
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
    const auto rows = txn.exec_params("SELECT from_uid FROM chat_group_msg WHERE group_id = $1 AND msg_id = $2 LIMIT 1",
                                      group_id,
                                      msg_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (rows.empty())
    {
        txn.abort();

        return false;
    }
    const int from_uid = rows[0]["from_uid"].as<int>();
    const bool has_delete_permission =
        from_uid != operator_uid &&
        HasGroupPermission(group_id, operator_uid, postgres_dao_group_messages_modules::DeleteMessagesPermissionBit());
    if (!postgres_dao_group_messages_modules::CanOperatorEditGroupMessage(from_uid,
                                                                          operator_uid,
                                                                          has_delete_permission))
    {
        txn.abort();

        return false;
    }

    const auto updated =
        txn.exec_params("UPDATE chat_group_msg SET content = $1, msg_type = 'text', mentions_json = '[]', "
                        "edited_at_ms = $2, deleted_at_ms = 0 WHERE group_id = $3 AND msg_id = $4",
                        content,
                        edited_at_ms,
                        group_id,
                        msg_id);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (updated.affected_rows() <= 0)
    {
        txn.abort();

        return false;
    }
    txn.exec_params("DELETE FROM chat_group_msg_ext WHERE msg_id = $1", msg_id);
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

bool PostgresDao::RevokeGroupMessage(const int64_t& group_id,
                                     const int& operator_uid,
                                     const std::string& msg_id,
                                     int64_t deleted_at_ms)
{
    if (!postgres_dao_group_messages_modules::CanRequestGroupMessageRevoke(group_id, operator_uid, msg_id.empty()))
    {
        return false;
    }
    if (postgres_dao_group_messages_modules::ShouldUseFallbackTimestamp(deleted_at_ms))
    {
        deleted_at_ms = static_cast<int64_t>(
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
    const auto rows = txn.exec_params(
        "SELECT from_uid, created_at, deleted_at_ms FROM chat_group_msg WHERE group_id = $1 AND msg_id = $2 "
        "LIMIT 1",
        group_id,
        msg_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (rows.empty())
    {
        txn.abort();

        return false;
    }
    const int from_uid = rows[0]["from_uid"].as<int>();
    const int64_t created_at = rows[0]["created_at"].as<int64_t>();
    const int64_t existing_deleted_at_ms =
        rows[0]["deleted_at_ms"].is_null() ? 0 : rows[0]["deleted_at_ms"].as<int64_t>();
    const long long revoke_window_ms = postgres_dao_group_messages_modules::MessageRevokeWindowMs();
    if (!postgres_dao_group_messages_modules::CanRevokeGroupMessage(from_uid,
                                                                    operator_uid,
                                                                    existing_deleted_at_ms,
                                                                    created_at,
                                                                    deleted_at_ms,
                                                                    revoke_window_ms))
    {
        txn.abort();

        return false;
    }

    const auto updated = txn.exec_params(
        "UPDATE chat_group_msg SET content = '[消息已撤回]', msg_type = 'revoke', mentions_json = '[]', "
        "deleted_at_ms = $1, edited_at_ms = 0 WHERE group_id = $2 AND msg_id = $3 AND from_uid = $4 "
        "AND deleted_at_ms = 0 AND created_at >= $5",
        deleted_at_ms,
        group_id,
        msg_id,
        operator_uid,
        postgres_dao_group_messages_modules::RevokeWindowStart(deleted_at_ms, revoke_window_ms));
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (updated.affected_rows() <= 0)
    {
        txn.abort();

        return false;
    }
    txn.exec_params("DELETE FROM chat_group_msg_ext WHERE msg_id = $1", msg_id);
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

bool PostgresDao::GetGroupHistory(const int64_t& group_id,
                                  const int64_t& before_ts,
                                  const int64_t& before_seq,
                                  const int& limit,
                                  std::vector<std::shared_ptr<GroupMessageInfo>>& messages,
                                  bool& has_more)
{
    has_more = false;
    messages.clear();

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const int final_limit = postgres_dao_group_messages_modules::ClampHistoryLimit(limit);
    const int fetch_limit = postgres_dao_group_messages_modules::SelectHistoryFetchLimit(final_limit);
    pqxx::result rows;
    if (postgres_dao_group_messages_modules::ShouldApplyGroupSeqCursor(before_seq))
    {
        rows = txn.exec_params(
            "SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, "
            "m.mentions_json, "
            "m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
            "e.file_name, e.mime, e.size "
            "FROM chat_group_msg m "
            "LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
            "WHERE m.group_id = $1 AND m.group_seq < $2 "
            "ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT $3",
            group_id,
            before_seq,
            fetch_limit);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    else if (postgres_dao_group_messages_modules::ShouldApplyTimestampCursor(before_seq, before_ts))
    {
        rows = txn.exec_params(
            "SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, "
            "m.mentions_json, "
            "m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
            "e.file_name, e.mime, e.size "
            "FROM chat_group_msg m "
            "LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
            "WHERE m.group_id = $1 AND m.created_at < $2 "
            "ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT $3",
            group_id,
            before_ts,
            fetch_limit);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    else
    {
        rows = txn.exec_params(
            "SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, "
            "m.mentions_json, "
            "m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
            "e.file_name, e.mime, e.size "
            "FROM chat_group_msg m "
            "LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
            "WHERE m.group_id = $1 ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT $2",
            group_id,
            fetch_limit);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }

    for (const auto& row : rows)
    {
        auto info = std::make_shared<GroupMessageInfo>();
        info->msg_id = row["msg_id"].c_str();
        info->group_id = row["group_id"].as<int64_t>();
        info->from_uid = row["from_uid"].as<int>();
        info->msg_type = row["msg_type"].is_null() ? "" : row["msg_type"].c_str();
        info->content = row["content"].is_null() ? "" : row["content"].c_str();
        info->mentions_json = row["mentions_json"].is_null() ? "" : row["mentions_json"].c_str();
        info->reply_to_server_msg_id =
            row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
        info->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
        info->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
        info->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
        info->created_at = row["created_at"].as<int64_t>();
        info->server_msg_id = row["server_msg_id"].as<int64_t>();
        info->group_seq = row["group_seq"].as<int64_t>();
        info->file_name = row["file_name"].is_null() ? "" : row["file_name"].c_str();
        info->mime = row["mime"].is_null() ? "" : row["mime"].c_str();
        info->size = row["size"].is_null() ? 0 : row["size"].as<int>();
        // Author base-info resolved in a batch below (account-data seam,
        // replaces the former LEFT JOIN "user").
        messages.push_back(info);
    }
    if (postgres_dao_group_messages_modules::HasOverflowPage(static_cast<int>(messages.size()), final_limit))
    {
        has_more = true;
        messages.resize(final_limit);
    }
    // Step 2: batch-resolve author name/nick/icon (no cross-table JOIN).
    std::vector<int> author_uids;
    author_uids.reserve(messages.size());
    for (const auto& m : messages)
    {
        if (m && m->from_uid != 0)
        {
            author_uids.push_back(m->from_uid);
        }
    }
    auto authors = GetUsersByUids(author_uids);
    for (auto& m : messages)
    {
        if (!m)
        {
            continue;
        }
        const auto it = authors.find(m->from_uid);
        if (it != authors.end() && it->second)
        {
            m->from_name = it->second->name;
            m->from_nick = it->second->nick;
            m->from_icon = it->second->icon;
        }
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetGroupMessageByMsgId(const int64_t& group_id,
                                         const std::string& msg_id,
                                         std::shared_ptr<GroupMessageInfo>& message)
{
    message = nullptr;
    if (!postgres_dao_group_messages_modules::CanFindGroupMessage(group_id, msg_id.empty()))
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
        txn.exec_params("SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, "
                        "m.mentions_json, "
                        "m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
                        "e.file_name, e.mime, e.size "
                        "FROM chat_group_msg m "
                        "LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
                        "WHERE m.group_id = $1 AND m.msg_id = $2 LIMIT 1",
                        group_id,
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
    auto one = std::make_shared<GroupMessageInfo>();
    one->msg_id = row["msg_id"].c_str();
    one->group_id = row["group_id"].as<int64_t>();
    one->from_uid = row["from_uid"].as<int>();
    one->msg_type = row["msg_type"].is_null() ? "" : row["msg_type"].c_str();
    one->content = row["content"].is_null() ? "" : row["content"].c_str();
    one->mentions_json = row["mentions_json"].is_null() ? "[]" : row["mentions_json"].c_str();
    one->reply_to_server_msg_id =
        row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
    one->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
    one->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
    one->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
    one->created_at = row["created_at"].as<int64_t>();
    one->server_msg_id = row["server_msg_id"].as<int64_t>();
    one->group_seq = row["group_seq"].as<int64_t>();
    one->file_name = row["file_name"].is_null() ? "" : row["file_name"].c_str();
    one->mime = row["mime"].is_null() ? "" : row["mime"].c_str();
    one->size = row["size"].is_null() ? 0 : row["size"].as<int>();
    message = one;
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}
