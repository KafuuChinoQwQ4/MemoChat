#include "PostgresDao.h"
#include "db/PqxxCompat.h"
#include "PostgresPool.h"
#include <pqxx/pqxx>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <sstream>

namespace
{
constexpr int64_t kMessageRevokeWindowMsPostgresDao = 5 * 60 * 1000;

int64_t NowMsPostgresDao()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}
} // namespace
bool PostgresDao::SavePrivateMessage(const PrivateMessageInfo& msg)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto result = txn.exec_params(
                "INSERT INTO chat_private_msg(msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
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
            txn.commit();
            return result.affected_rows() >= 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "INSERT INTO chat_private_msg(msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
            "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
            "VALUES(?,?,?,?,?,?,?,?,?,?,?) "
            "ON DUPLICATE KEY UPDATE from_uid = VALUES(from_uid), to_uid = VALUES(to_uid), "
            "content = VALUES(content), reply_to_server_msg_id = VALUES(reply_to_server_msg_id), "
            "forward_meta_json = VALUES(forward_meta_json), edited_at_ms = VALUES(edited_at_ms), "
            "deleted_at_ms = VALUES(deleted_at_ms), created_at = VALUES(created_at)"));
        pstmt->setString(1, msg.msg_id);
        pstmt->setInt(2, msg.conv_uid_min);
        pstmt->setInt(3, msg.conv_uid_max);
        pstmt->setInt(4, msg.from_uid);
        pstmt->setInt(5, msg.to_uid);
        pstmt->setString(6, msg.content);
        pstmt->setInt64(7, msg.reply_to_server_msg_id);
        pstmt->setString(8, msg.forward_meta_json);
        pstmt->setInt64(9, msg.edited_at_ms);
        pstmt->setInt64(10, msg.deleted_at_ms);
        pstmt->setInt64(11, msg.created_at);
        const int row_affected = pstmt->executeUpdate();
        return row_affected >= 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::GetUndeliveredPrivateMessages(const int& to_uid,
                                                const int64_t& after_created_at,
                                                const std::string& after_msg_id,
                                                int limit,
                                                std::vector<std::shared_ptr<PrivateMessageInfo>>& messages)
{
    messages.clear();
    if (to_uid <= 0 || limit <= 0)
    {
        return false;
    }

    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
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
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
            "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
            "FROM chat_private_msg "
            "WHERE to_uid = ? AND (created_at > ? OR (created_at = ? AND msg_id > ?)) AND deleted_at_ms = 0 "
            "AND created_at > COALESCE(( "
            "    SELECT read_ts FROM chat_private_read_state "
            "    WHERE uid = ? AND peer_uid = chat_private_msg.from_uid "
            "), 0) "
            "ORDER BY created_at ASC, msg_id ASC "
            "LIMIT ?"));
        pstmt->setString(1, std::to_string(to_uid));
        pstmt->setInt64(2, after_created_at);
        pstmt->setInt64(3, after_created_at);
        pstmt->setString(4, after_msg_id);
        pstmt->setInt(5, to_uid);
        pstmt->setInt(6, limit);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            auto msg = std::make_shared<PrivateMessageInfo>();
            msg->msg_id = res->isNull("msg_id") ? "" : res->getString("msg_id");
            msg->conv_uid_min = res->getInt("conv_uid_min");
            msg->conv_uid_max = res->getInt("conv_uid_max");
            msg->from_uid = res->getInt("from_uid");
            msg->to_uid = res->getInt("to_uid");
            msg->content = res->isNull("content") ? "" : res->getString("content");
            msg->reply_to_server_msg_id =
                res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
            msg->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
            msg->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
            msg->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
            msg->created_at = res->isNull("created_at") ? 0 : res->getInt64("created_at");
            messages.push_back(msg);
        }
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
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
    if (uid <= 0 || peer_uid <= 0 || limit <= 0)
    {
        return false;
    }

    const int conv_min = std::min(uid, peer_uid);
    const int conv_max = std::max(uid, peer_uid);

    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const int final_limit = limit + 1;
            pqxx::result rows;
            if (before_ts > 0 && !before_msg_id.empty())
            {
                rows = txn.exec_params(
                    "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
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
            }
            else if (before_ts > 0)
            {
                rows = txn.exec_params(
                    "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                    "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                    "FROM chat_private_msg WHERE conv_uid_min = $1 AND conv_uid_max = $2 AND created_at < $3 "
                    "ORDER BY created_at DESC, msg_id DESC LIMIT $4",
                    conv_min,
                    conv_max,
                    before_ts,
                    final_limit);
            }
            else
            {
                rows = txn.exec_params(
                    "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                    "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                    "FROM chat_private_msg WHERE conv_uid_min = $1 AND conv_uid_max = $2 "
                    "ORDER BY created_at DESC, msg_id DESC LIMIT $3",
                    conv_min,
                    conv_max,
                    final_limit);
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

            if (static_cast<int>(messages.size()) > limit)
            {
                has_more = true;
                messages.resize(limit);
            }
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt;
        if (before_ts > 0 && !before_msg_id.empty())
        {
            pstmt.reset(con->_con->prepareStatement(
                "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                "FROM chat_private_msg WHERE conv_uid_min = ? AND conv_uid_max = ? "
                "AND (created_at < ? OR (created_at = ? AND msg_id < ?)) "
                "ORDER BY created_at DESC, msg_id DESC LIMIT ?"));
            pstmt->setInt(1, conv_min);
            pstmt->setInt(2, conv_max);
            pstmt->setInt64(3, before_ts);
            pstmt->setInt64(4, before_ts);
            pstmt->setString(5, before_msg_id);
            pstmt->setInt(6, limit + 1);
        }
        else if (before_ts > 0)
        {
            pstmt.reset(con->_con->prepareStatement(
                "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                "FROM chat_private_msg WHERE conv_uid_min = ? AND conv_uid_max = ? AND created_at < ? "
                "ORDER BY created_at DESC, msg_id DESC LIMIT ?"));
            pstmt->setInt(1, conv_min);
            pstmt->setInt(2, conv_max);
            pstmt->setInt64(3, before_ts);
            pstmt->setInt(4, limit + 1);
        }
        else
        {
            pstmt.reset(con->_con->prepareStatement(
                "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                "FROM chat_private_msg WHERE conv_uid_min = ? AND conv_uid_max = ? "
                "ORDER BY created_at DESC, msg_id DESC LIMIT ?"));
            pstmt->setInt(1, conv_min);
            pstmt->setInt(2, conv_max);
            pstmt->setInt(3, limit + 1);
        }

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            auto one = std::make_shared<PrivateMessageInfo>();
            one->msg_id = res->getString("msg_id");
            one->conv_uid_min = res->getInt("conv_uid_min");
            one->conv_uid_max = res->getInt("conv_uid_max");
            one->from_uid = res->getInt("from_uid");
            one->to_uid = res->getInt("to_uid");
            one->content = res->getString("content");
            one->reply_to_server_msg_id =
                res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
            one->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
            one->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
            one->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
            one->created_at = res->getInt64("created_at");
            messages.push_back(one);
        }

        if (static_cast<int>(messages.size()) > limit)
        {
            has_more = true;
            messages.resize(limit);
        }
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message)
{
    message = nullptr;
    if (msg_id.empty())
    {
        return false;
    }

    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows =
                txn.exec_params("SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
                                "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
                                "FROM chat_private_msg WHERE msg_id = $1 LIMIT 1",
                                msg_id);
            if (rows.empty())
            {
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
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
            "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
            "FROM chat_private_msg WHERE msg_id = ? LIMIT 1"));
        pstmt->setString(1, msg_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return false;
        }

        auto one = std::make_shared<PrivateMessageInfo>();
        one->msg_id = res->getString("msg_id");
        one->conv_uid_min = res->getInt("conv_uid_min");
        one->conv_uid_max = res->getInt("conv_uid_max");
        one->from_uid = res->getInt("from_uid");
        one->to_uid = res->getInt("to_uid");
        one->content = res->getString("content");
        one->reply_to_server_msg_id =
            res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
        one->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
        one->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
        one->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
        one->created_at = res->getInt64("created_at");
        message = one;
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::EnqueueChatOutboxEvent(const ChatOutboxEventInfo& event)
{
    if (!use_postgres_)
    {
        return false;
    }
    try
    {
        pqxx::connection conn(postgres_connection_string_);
        pqxx::work txn(conn);
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
                         event.created_at > 0 ? event.created_at : NowMsPostgresDao(),
                         event.published_at,
                         event.last_error);
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::GetPendingChatOutboxEvents(int limit, std::vector<ChatOutboxEventInfo>& events)
{
    events.clear();
    if (!use_postgres_ || limit <= 0)
    {
        return false;
    }
    try
    {
        pqxx::connection conn(postgres_connection_string_);
        pqxx::work txn(conn);
        const auto now_ms = NowMsPostgresDao();
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
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::MarkChatOutboxEventPublished(int64_t id, int64_t published_at_ms)
{
    if (!use_postgres_ || id <= 0)
    {
        return false;
    }
    try
    {
        pqxx::connection conn(postgres_connection_string_);
        pqxx::work txn(conn);
        const auto rows =
            txn.exec_params("UPDATE chat_event_outbox SET status = 1, published_at = $2, last_error = '' WHERE id = $1",
                            id,
                            published_at_ms > 0 ? published_at_ms : NowMsPostgresDao());
        txn.commit();
        return rows.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::MarkChatOutboxEventRetry(int64_t id,
                                           int retry_count,
                                           int64_t next_retry_at_ms,
                                           const std::string& last_error,
                                           bool terminal_error)
{
    if (!use_postgres_ || id <= 0)
    {
        return false;
    }
    try
    {
        pqxx::connection conn(postgres_connection_string_);
        pqxx::work txn(conn);
        const auto rows = txn.exec_params("UPDATE chat_event_outbox SET status = $2, retry_count = $3, next_retry_at = "
                                          "$4, last_error = $5 WHERE id = $1",
                                          id,
                                          terminal_error ? 2 : 0,
                                          retry_count,
                                          next_retry_at_ms,
                                          last_error);
        txn.commit();
        return rows.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::ExpediteChatOutboxEventRetry(int64_t id)
{
    if (!use_postgres_ || id <= 0)
    {
        return false;
    }
    try
    {
        pqxx::connection conn(postgres_connection_string_);
        pqxx::work txn(conn);
        const auto rows =
            txn.exec_params("UPDATE chat_event_outbox SET status = 0, next_retry_at = $2 WHERE id = $1 AND status <> 1",
                            id,
                            NowMsPostgresDao());
        txn.commit();
        return rows.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::UpdatePrivateMessageContent(const int& uid,
                                              const int& peer_uid,
                                              const std::string& msg_id,
                                              const std::string& content,
                                              int64_t edited_at_ms)
{
    if (uid <= 0 || peer_uid <= 0 || msg_id.empty() || content.empty())
    {
        return false;
    }
    if (edited_at_ms <= 0)
    {
        edited_at_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }
    if (use_postgres_)
    {
        try
        {
            std::shared_ptr<PrivateMessageInfo> message;
            if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
            {
                return false;
            }
            const int conv_min = std::min(uid, peer_uid);
            const int conv_max = std::max(uid, peer_uid);
            if (message->conv_uid_min != conv_min || message->conv_uid_max != conv_max || message->from_uid != uid)
            {
                return false;
            }

            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_private_msg SET content = $1, edited_at_ms = $2, deleted_at_ms = 0 "
                                "WHERE msg_id = $3 AND conv_uid_min = $4 AND conv_uid_max = $5",
                                content,
                                edited_at_ms,
                                msg_id,
                                conv_min,
                                conv_max);
            txn.commit();
            return updated.affected_rows() > 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::shared_ptr<PrivateMessageInfo> message;
        if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
        {
            return false;
        }
        const int conv_min = std::min(uid, peer_uid);
        const int conv_max = std::max(uid, peer_uid);
        if (message->conv_uid_min != conv_min || message->conv_uid_max != conv_max || message->from_uid != uid)
        {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "UPDATE chat_private_msg SET content = ?, edited_at_ms = ?, deleted_at_ms = 0, "
            "created_at = created_at WHERE msg_id = ? AND conv_uid_min = ? AND conv_uid_max = ?"));
        pstmt->setString(1, content);
        pstmt->setInt64(2, edited_at_ms);
        pstmt->setString(3, msg_id);
        pstmt->setInt(4, conv_min);
        pstmt->setInt(5, conv_max);
        return pstmt->executeUpdate() > 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::RevokePrivateMessage(const int& uid,
                                       const int& peer_uid,
                                       const std::string& msg_id,
                                       int64_t deleted_at_ms)
{
    if (uid <= 0 || peer_uid <= 0 || msg_id.empty())
    {
        return false;
    }
    if (deleted_at_ms <= 0)
    {
        deleted_at_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }
    if (use_postgres_)
    {
        try
        {
            std::shared_ptr<PrivateMessageInfo> message;
            if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
            {
                return false;
            }
            const int conv_min = std::min(uid, peer_uid);
            const int conv_max = std::max(uid, peer_uid);
            if (message->conv_uid_min != conv_min || message->conv_uid_max != conv_max || message->from_uid != uid ||
                message->deleted_at_ms > 0 || message->created_at <= 0 ||
                deleted_at_ms - message->created_at > kMessageRevokeWindowMsPostgresDao)
            {
                return false;
            }

            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated = txn.exec_params(
                "UPDATE chat_private_msg SET content = '[消息已撤回]', deleted_at_ms = $1, edited_at_ms = 0 "
                "WHERE msg_id = $2 AND conv_uid_min = $3 AND conv_uid_max = $4 AND from_uid = $5 "
                "AND deleted_at_ms = 0 AND created_at >= $6",
                deleted_at_ms,
                msg_id,
                conv_min,
                conv_max,
                uid,
                deleted_at_ms - kMessageRevokeWindowMsPostgresDao);
            txn.commit();
            return updated.affected_rows() > 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::shared_ptr<PrivateMessageInfo> message;
        if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
        {
            return false;
        }
        const int conv_min = std::min(uid, peer_uid);
        const int conv_max = std::max(uid, peer_uid);
        if (message->conv_uid_min != conv_min || message->conv_uid_max != conv_max || message->from_uid != uid ||
            message->deleted_at_ms > 0 || message->created_at <= 0 ||
            deleted_at_ms - message->created_at > kMessageRevokeWindowMsPostgresDao)
        {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "UPDATE chat_private_msg SET content = '[消息已撤回]', deleted_at_ms = ?, edited_at_ms = 0, "
            "created_at = created_at WHERE msg_id = ? AND conv_uid_min = ? AND conv_uid_max = ? AND from_uid = ? "
            "AND deleted_at_ms = 0 AND created_at >= ?"));
        pstmt->setInt64(1, deleted_at_ms);
        pstmt->setString(2, msg_id);
        pstmt->setInt(3, conv_min);
        pstmt->setInt(4, conv_max);
        pstmt->setInt(5, uid);
        pstmt->setInt64(6, deleted_at_ms - kMessageRevokeWindowMsPostgresDao);
        return pstmt->executeUpdate() > 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::UpsertPrivateReadState(const int& uid, const int& peer_uid, const int64_t& read_ts)
{
    if (uid <= 0 || peer_uid <= 0)
    {
        return false;
    }
    int64_t normalized_read_ts = read_ts;
    if (normalized_read_ts <= 0)
    {
        normalized_read_ts = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto result =
                txn.exec_params("INSERT INTO chat_private_read_state(uid, peer_uid, read_ts) VALUES($1, $2, $3) "
                                "ON CONFLICT (uid, peer_uid) DO UPDATE SET "
                                "read_ts = GREATEST(chat_private_read_state.read_ts, EXCLUDED.read_ts), "
                                "updated_at = CURRENT_TIMESTAMP",
                                uid,
                                peer_uid,
                                normalized_read_ts);
            txn.commit();
            return result.affected_rows() >= 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "INSERT INTO chat_private_read_state(uid, peer_uid, read_ts) VALUES(?, ?, ?) "
            "ON DUPLICATE KEY UPDATE read_ts = GREATEST(read_ts, VALUES(read_ts)), updated_at = CURRENT_TIMESTAMP"));
        pstmt->setInt(1, uid);
        pstmt->setInt(2, peer_uid);
        pstmt->setInt64(3, normalized_read_ts);
        return pstmt->executeUpdate() >= 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}
