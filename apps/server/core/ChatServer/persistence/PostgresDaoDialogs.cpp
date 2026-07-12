#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "PostgresDaoUtil.hpp"
#include "db/PqxxCompat.hpp"
#include "SnowflakeUtil.hpp"
#include <set>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <limits>
#include <sstream>

import memochat.chat.postgres_dao_dialogs_algorithms;

namespace postgres_dao_dialogs_modules = memochat::chat::persistence::postgres_dao_dialogs::modules;

namespace
{
constexpr int64_t kMessageIdempotencySchemaLock = 0x4D43484944000001LL;
constexpr int64_t kEventOutboxSchemaLock = 0x4D43484F55540001LL;

bool IsValidGroupCode(const std::string& group_code)
{
    const int length = static_cast<int>(group_code.size());
    const char prefix = length > 0 ? group_code[0] : '\0';
    const char first_digit = length > 1 ? group_code[1] : '\0';
    if (!postgres_dao_dialogs_modules::HasGroupCodeHeader(length, prefix, first_digit))
    {
        return false;
    }
    for (size_t i = 2; i < group_code.size(); ++i)
    {
        if (!postgres_dao_dialogs_modules::IsGroupCodeTailChar(group_code[i]))
        {
            return false;
        }
    }
    return true;
}

} // namespace
bool PostgresDao::GetDialogMetaByOwner(const int& owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas)
{
    metas.clear();
    if (!postgres_dao_dialogs_modules::CanLoadDialogMeta(owner_uid))
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
    const auto rows = txn.exec_params("SELECT dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state "
                                      "FROM chat_dialog WHERE owner_uid = $1",
                                      owner_uid);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    for (const auto& row : rows)
    {
        auto info = std::make_shared<DialogMetaInfo>();
        info->owner_uid = owner_uid;
        info->dialog_type = row["dialog_type"].is_null() ? "" : row["dialog_type"].c_str();
        info->peer_uid = row["peer_uid"].is_null() ? 0 : row["peer_uid"].as<int>();
        info->group_id = row["group_id"].is_null() ? 0 : row["group_id"].as<int64_t>();
        info->pinned_rank = row["pinned_rank"].is_null() ? 0 : row["pinned_rank"].as<int>();
        info->draft_text = row["draft_text"].is_null() ? "" : row["draft_text"].c_str();
        info->mute_state = row["mute_state"].is_null() ? 0 : row["mute_state"].as<int>();
        metas.push_back(info);
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetPrivateDialogRuntime(const int& owner_uid, const int& peer_uid, DialogRuntimeInfo& runtime)
{
    runtime = DialogRuntimeInfo();
    if (!postgres_dao_dialogs_modules::CanReadPrivateDialogRuntime(owner_uid, peer_uid))
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
        txn.exec_params("SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
                        "WHERE owner_uid = $1 AND dialog_type = 'private' AND peer_uid = $2 AND group_id = 0 LIMIT 1",
                        owner_uid,
                        peer_uid);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!rows.empty())
    {
        const auto& row = rows[0];
        runtime.last_msg_preview = row["last_msg_preview"].is_null() ? "" : row["last_msg_preview"].c_str();
        runtime.last_msg_ts = row["last_msg_ts"].is_null() ? 0 : row["last_msg_ts"].as<int64_t>();
        runtime.unread_count = row["unread_count"].is_null()
                                   ? 0
                                   : postgres_dao_dialogs_modules::NormalizeUnreadCount(row["unread_count"].as<int>());
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetGroupDialogRuntime(const int& owner_uid, const int64_t& group_id, DialogRuntimeInfo& runtime)
{
    runtime = DialogRuntimeInfo();
    if (!postgres_dao_dialogs_modules::CanReadGroupDialogRuntime(owner_uid, group_id))
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
    const auto rows = txn.exec_params("SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
                                      "WHERE owner_uid = $1 AND dialog_type = 'group' AND group_id = $2 LIMIT 1",
                                      owner_uid,
                                      group_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!rows.empty())
    {
        const auto& row = rows[0];
        runtime.last_msg_preview = row["last_msg_preview"].is_null() ? "" : row["last_msg_preview"].c_str();
        runtime.last_msg_ts = row["last_msg_ts"].is_null() ? 0 : row["last_msg_ts"].as<int64_t>();
        runtime.unread_count = row["unread_count"].is_null()
                                   ? 0
                                   : postgres_dao_dialogs_modules::NormalizeUnreadCount(row["unread_count"].as<int>());
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::RefreshDialogsForOwner(const int& owner_uid)
{
    if (!postgres_dao_dialogs_modules::CanLoadDialogMeta(owner_uid))
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
    std::vector<int> peers;
    {
        const auto peer_rows = txn.exec_params("SELECT friend_id FROM friend WHERE self_id = $1", owner_uid);
        if (!peer_rows.ok())
        {
            const auto& postgres_error = peer_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        for (const auto& row : peer_rows)
        {
            const auto peer_uid = row["friend_id"].as<int>();
            if (postgres_dao_dialogs_modules::ShouldKeepPositiveUid(peer_uid))
            {
                peers.push_back(peer_uid);
            }
        }
    }

    for (int peer_uid : peers)
    {
        txn.exec_params0("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, "
                         "draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
                         "VALUES($1, 'private', $2, 0, 0, '', 0, '', 0, 0) "
                         "ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO NOTHING",
                         owner_uid,
                         peer_uid);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }

        const int conv_min = postgres_dao_dialogs_modules::ConversationUidMin(owner_uid, peer_uid);
        const int conv_max = postgres_dao_dialogs_modules::ConversationUidMax(owner_uid, peer_uid);
        std::string preview;
        int64_t last_msg_ts = 0;
        int unread_count = 0;

        const auto last_rows = txn.exec_params("SELECT content, created_at FROM chat_private_msg "
                                               "WHERE conv_uid_min = $1 AND conv_uid_max = $2 "
                                               "ORDER BY created_at DESC, msg_id DESC LIMIT 1",
                                               conv_min,
                                               conv_max);
        if (!last_rows.ok())
        {
            const auto& postgres_error = last_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        if (!last_rows.empty())
        {
            const auto content = last_rows[0]["content"].is_null() ? "" : std::string(last_rows[0]["content"].c_str());
            preview = BuildPreviewText("", content);
            last_msg_ts = last_rows[0]["created_at"].is_null() ? 0 : last_rows[0]["created_at"].as<int64_t>();
        }

        const auto unread_rows =
            txn.exec_params("SELECT COUNT(1) AS unread_count FROM chat_private_msg "
                            "WHERE conv_uid_min = $1 AND conv_uid_max = $2 AND from_uid = $3 "
                            "AND created_at > COALESCE((SELECT read_ts FROM chat_private_read_state WHERE uid "
                            "= $4 AND peer_uid = $5), 0)",
                            conv_min,
                            conv_max,
                            peer_uid,
                            owner_uid,
                            peer_uid);
        if (!unread_rows.ok())
        {
            const auto& postgres_error = unread_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        if (!unread_rows.empty())
        {
            unread_count = postgres_dao_dialogs_modules::NormalizeUnreadCount(unread_rows[0]["unread_count"].as<int>());
        }

        txn.exec_params0("UPDATE chat_dialog SET last_msg_preview = $1, last_msg_ts = $2, unread_count = $3, "
                         "updated_at = CURRENT_TIMESTAMP "
                         "WHERE owner_uid = $4 AND dialog_type = 'private' AND peer_uid = $5 AND group_id = 0",
                         preview,
                         last_msg_ts,
                         unread_count,
                         owner_uid,
                         peer_uid);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }

    std::vector<int64_t> groups;
    {
        const auto group_rows = txn.exec_params("SELECT m.group_id FROM chat_group_member m "
                                                "JOIN chat_group g ON g.group_id = m.group_id "
                                                "WHERE m.uid = $1 AND m.status = 1 AND g.status = 1",
                                                owner_uid);
        if (!group_rows.ok())
        {
            const auto& postgres_error = group_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        for (const auto& row : group_rows)
        {
            const auto group_id = row["group_id"].as<int64_t>();
            if (postgres_dao_dialogs_modules::ShouldKeepPositiveGroupId(group_id))
            {
                groups.push_back(group_id);
            }
        }
    }

    for (int64_t group_id : groups)
    {
        txn.exec_params0("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, "
                         "draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
                         "VALUES($1, 'group', 0, $2, 0, '', 0, '', 0, 0) "
                         "ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO NOTHING",
                         owner_uid,
                         group_id);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }

        std::string preview;
        int64_t last_msg_ts = 0;
        int unread_count = 0;

        const auto last_rows =
            txn.exec_params("SELECT msg_type, content, created_at FROM chat_group_msg "
                            "WHERE group_id = $1 ORDER BY group_seq DESC, server_msg_id DESC LIMIT 1",
                            group_id);
        if (!last_rows.ok())
        {
            const auto& postgres_error = last_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        if (!last_rows.empty())
        {
            const auto msg_type =
                last_rows[0]["msg_type"].is_null() ? "" : std::string(last_rows[0]["msg_type"].c_str());
            const auto content = last_rows[0]["content"].is_null() ? "" : std::string(last_rows[0]["content"].c_str());
            preview = BuildPreviewText(msg_type, content);
            last_msg_ts = last_rows[0]["created_at"].is_null() ? 0 : last_rows[0]["created_at"].as<int64_t>();
        }

        const auto unread_rows = txn.exec_params("SELECT COUNT(1) AS unread_count FROM chat_group_msg "
                                                 "WHERE group_id = $1 AND created_at > COALESCE((SELECT read_ts FROM "
                                                 "chat_group_read_state WHERE uid = $2 AND group_id = $3), 0) "
                                                 "AND from_uid <> $4",
                                                 group_id,
                                                 owner_uid,
                                                 group_id,
                                                 owner_uid);
        if (!unread_rows.ok())
        {
            const auto& postgres_error = unread_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        if (!unread_rows.empty())
        {
            unread_count = postgres_dao_dialogs_modules::NormalizeUnreadCount(unread_rows[0]["unread_count"].as<int>());
        }

        txn.exec_params0("UPDATE chat_dialog SET last_msg_preview = $1, last_msg_ts = $2, unread_count = $3, "
                         "updated_at = CURRENT_TIMESTAMP "
                         "WHERE owner_uid = $4 AND dialog_type = 'group' AND group_id = $5",
                         preview,
                         last_msg_ts,
                         unread_count,
                         owner_uid,
                         group_id);
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }

    txn.exec_params0(
        "DELETE FROM chat_dialog "
        "WHERE owner_uid = $1 AND dialog_type = 'private' AND peer_uid > 0 "
        "AND NOT EXISTS (SELECT 1 FROM friend f WHERE f.self_id = $2 AND f.friend_id = chat_dialog.peer_uid)",
        owner_uid,
        owner_uid);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params0("DELETE FROM chat_dialog "
                     "WHERE owner_uid = $1 AND dialog_type = 'group' AND group_id > 0 "
                     "AND NOT EXISTS ("
                     "SELECT 1 FROM chat_group_member m JOIN chat_group g ON g.group_id = m.group_id "
                     "WHERE m.uid = $2 AND m.group_id = chat_dialog.group_id AND m.status = 1 AND g.status = 1)",
                     owner_uid,
                     owner_uid);
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

bool PostgresDao::UpsertGroupReadState(const int& uid, const int64_t& group_id, const int64_t& read_ts)
{
    if (!postgres_dao_dialogs_modules::CanUpsertGroupReadState(uid, group_id))
    {
        return false;
    }
    int64_t normalized_read_ts = read_ts;
    if (postgres_dao_dialogs_modules::ShouldUseFallbackTimestamp(normalized_read_ts))
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
    const auto result = txn.exec_params("INSERT INTO chat_group_read_state(uid, group_id, read_ts) VALUES($1, $2, $3) "
                                        "ON CONFLICT (uid, group_id) DO UPDATE SET "
                                        "read_ts = GREATEST(chat_group_read_state.read_ts, EXCLUDED.read_ts), "
                                        "updated_at = CURRENT_TIMESTAMP",
                                        uid,
                                        group_id,
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

bool PostgresDao::UpsertDialogDraft(const int& owner_uid,
                                    const std::string& dialog_type,
                                    const int& peer_uid,
                                    const int64_t& group_id,
                                    const std::string& draft_text)
{
    const bool is_private = dialog_type == "private";
    const bool is_group = dialog_type == "group";
    const int normalized_peer_uid = postgres_dao_dialogs_modules::NormalizeDialogPeerUid(is_private, peer_uid);
    const int64_t normalized_group_id = postgres_dao_dialogs_modules::NormalizeDialogGroupId(is_group, group_id);
    if (!postgres_dao_dialogs_modules::CanUpsertDialogMeta(owner_uid,
                                                           is_private,
                                                           is_group,
                                                           normalized_peer_uid,
                                                           normalized_group_id))
    {
        return false;
    }

    std::string normalized_draft = draft_text;
    if (normalized_draft.size() > static_cast<size_t>(postgres_dao_dialogs_modules::MaxDraftLength()))
    {
        normalized_draft.resize(static_cast<size_t>(postgres_dao_dialogs_modules::MaxDraftLength()));
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, "
                    "draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
                    "VALUES($1, $2, $3, $4, 0, $5, 0, '', 0, 0) "
                    "ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO UPDATE SET "
                    "draft_text = EXCLUDED.draft_text, updated_at = CURRENT_TIMESTAMP",
                    owner_uid,
                    dialog_type,
                    normalized_peer_uid,
                    normalized_group_id,
                    normalized_draft);
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

bool PostgresDao::UpsertDialogPinned(const int& owner_uid,
                                     const std::string& dialog_type,
                                     const int& peer_uid,
                                     const int64_t& group_id,
                                     const int& pinned_rank)
{
    const bool is_private = dialog_type == "private";
    const bool is_group = dialog_type == "group";
    const int normalized_peer_uid = postgres_dao_dialogs_modules::NormalizeDialogPeerUid(is_private, peer_uid);
    const int64_t normalized_group_id = postgres_dao_dialogs_modules::NormalizeDialogGroupId(is_group, group_id);
    if (!postgres_dao_dialogs_modules::CanUpsertDialogMeta(owner_uid,
                                                           is_private,
                                                           is_group,
                                                           normalized_peer_uid,
                                                           normalized_group_id))
    {
        return false;
    }
    const int normalized_pinned_rank = postgres_dao_dialogs_modules::NormalizePinnedRank(pinned_rank);

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, "
                    "draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
                    "VALUES($1, $2, $3, $4, $5, '', 0, '', 0, 0) "
                    "ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO UPDATE SET "
                    "pinned_rank = EXCLUDED.pinned_rank, updated_at = CURRENT_TIMESTAMP",
                    owner_uid,
                    dialog_type,
                    normalized_peer_uid,
                    normalized_group_id,
                    normalized_pinned_rank);
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

bool PostgresDao::UpsertDialogMuteState(const int& owner_uid,
                                        const std::string& dialog_type,
                                        const int& peer_uid,
                                        const int64_t& group_id,
                                        const int& mute_state)
{
    const bool is_private = dialog_type == "private";
    const bool is_group = dialog_type == "group";
    const int normalized_peer_uid = postgres_dao_dialogs_modules::NormalizeDialogPeerUid(is_private, peer_uid);
    const int64_t normalized_group_id = postgres_dao_dialogs_modules::NormalizeDialogGroupId(is_group, group_id);
    if (!postgres_dao_dialogs_modules::CanUpsertDialogMeta(owner_uid,
                                                           is_private,
                                                           is_group,
                                                           normalized_peer_uid,
                                                           normalized_group_id))
    {
        return false;
    }
    const int normalized_mute_state = postgres_dao_dialogs_modules::NormalizeMuteState(mute_state);

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, "
                    "draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
                    "VALUES($1, $2, $3, $4, 0, '', $5, '', 0, 0) "
                    "ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO UPDATE SET "
                    "mute_state = EXCLUDED.mute_state, updated_at = CURRENT_TIMESTAMP",
                    owner_uid,
                    dialog_type,
                    normalized_peer_uid,
                    normalized_group_id,
                    normalized_mute_state);
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

std::string PostgresDao::GenerateGroupCode()
{
    return SnowflakeUtil::formatPublicId(SnowflakeUtil::getInstance().nextId(), 'g');
}

bool PostgresDao::EnsureChatMessageIdempotencySchema()
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "EnsureChatMessageIdempotencySchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto schema_lock = txn.exec_params("SELECT pg_advisory_xact_lock($1)", kMessageIdempotencySchemaLock);
    if (!schema_lock.ok())
    {
        const auto& postgres_error = schema_lock.error_message();
        std::cerr << "EnsureChatMessageIdempotencySchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec0("CREATE UNIQUE INDEX IF NOT EXISTS uk_chat_private_msg_msg_id ON chat_private_msg(msg_id)");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatMessageIdempotencySchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec0("CREATE UNIQUE INDEX IF NOT EXISTS uk_chat_group_msg_group_msg_id ON chat_group_msg(group_id, msg_id)");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatMessageIdempotencySchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatMessageIdempotencySchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::EnsureChatEventOutboxSchema()
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "EnsureChatEventOutboxSchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto schema_lock = txn.exec_params("SELECT pg_advisory_xact_lock($1)", kEventOutboxSchemaLock);
    if (!schema_lock.ok())
    {
        const auto& postgres_error = schema_lock.error_message();
        std::cerr << "EnsureChatEventOutboxSchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec0("CREATE TABLE IF NOT EXISTS chat_event_outbox ("
              "id BIGSERIAL PRIMARY KEY,"
              "event_id VARCHAR(64) NOT NULL,"
              "topic VARCHAR(128) NOT NULL,"
              "partition_key VARCHAR(128) NOT NULL,"
              "payload_json JSONB NOT NULL,"
              "status SMALLINT NOT NULL DEFAULT 0,"
              "retry_count INT NOT NULL DEFAULT 0,"
              "next_retry_at BIGINT NOT NULL DEFAULT 0,"
              "created_at BIGINT NOT NULL,"
              "published_at BIGINT NULL,"
              "last_error TEXT NOT NULL DEFAULT '',"
              "CONSTRAINT uq_chat_event_outbox_event_id UNIQUE(event_id)"
              ")");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatEventOutboxSchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec0("CREATE INDEX IF NOT EXISTS idx_chat_event_outbox_status_retry ON chat_event_outbox(status, "
              "next_retry_at, id)");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatEventOutboxSchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec0("CREATE INDEX IF NOT EXISTS idx_chat_event_outbox_topic_status ON chat_event_outbox(topic, status, id)");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatEventOutboxSchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "EnsureChatEventOutboxSchema PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetPendingGroupApplyForReviewer(const int& reviewer_uid,
                                                  std::vector<std::shared_ptr<GroupApplyInfo>>& applies,
                                                  int limit)
{
    const int final_limit = postgres_dao_dialogs_modules::ClampGroupApplyLimit(limit);
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("SELECT a.apply_id, a.group_id, a.applicant_uid, a.inviter_uid, a.type, a.status, "
                        "a.reason, a.reviewer_uid "
                        "FROM chat_group_apply a "
                        "JOIN chat_group_member m ON a.group_id = m.group_id "
                        "LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
                        "WHERE a.status = 0 AND m.uid = $1 AND m.status = 1 AND "
                        "(m.role = 3 OR ((COALESCE(p.permission_bits, $2) & $3) <> 0)) "
                        "ORDER BY a.created_at DESC LIMIT $4",
                        reviewer_uid,
                        kDefaultAdminPermBits,
                        kPermInviteUsers,
                        final_limit);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    applies.clear();
    for (const auto& row : rows)
    {
        auto info = std::make_shared<GroupApplyInfo>();
        info->apply_id = row["apply_id"].as<int64_t>();
        info->group_id = row["group_id"].as<int64_t>();
        info->applicant_uid = row["applicant_uid"].is_null() ? 0 : row["applicant_uid"].as<int>();
        info->inviter_uid = row["inviter_uid"].is_null() ? 0 : row["inviter_uid"].as<int>();
        info->type = row["type"].is_null() ? "apply" : ((row["type"].as<int>() == 1) ? "invite" : "apply");
        info->status = row["status"].is_null() ? 0 : row["status"].as<int>();
        info->reason = row["reason"].is_null() ? "" : row["reason"].c_str();
        info->reviewer_uid = row["reviewer_uid"].is_null() ? 0 : row["reviewer_uid"].as<int>();
        applies.push_back(info);
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}
