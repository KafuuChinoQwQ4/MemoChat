#include "db/AISessionRepo.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include "logging/Logger.hpp"
#include "random/Uuid.hpp"
#include <chrono>
#include <mutex>
#include <sstream>

import memochat.ai.repository_algorithms;

namespace
{
namespace repository_modules = memochat::ai::repository::modules;

std::unique_ptr<pqxx::connection> GetPgConn(std::string& error)
{
    auto& cfg = ConfigMgr::Inst();
    const auto schema = repository_modules::ShouldUseDefaultSchema(cfg["Postgres"]["Schema"].empty())
                            ? repository_modules::DefaultPostgresSchema()
                            : cfg["Postgres"]["Schema"];
    std::ostringstream conn_str;
    conn_str << "host=" << cfg["Postgres"]["Host"] << " port=" << cfg["Postgres"]["Port"]
             << " user=" << cfg["Postgres"]["User"] << " password=" << cfg["Postgres"]["Passwd"]
             << " dbname=" << cfg["Postgres"]["Database"];
    auto conn = std::make_unique<pqxx::connection>(conn_str.str());
    if (!conn->is_open())
    {
        error = conn->error_message();
        return nullptr;
    }
    pqxx::work tx(*conn);
    const auto quoted_schema = tx.quote_name(schema);
    const auto configured = tx.exec0("SET search_path TO " + quoted_schema + ",public");
    if (!configured.ok() || !tx.commit())
    {
        error = configured.ok() ? tx.error_message() : configured.error_message();
        return nullptr;
    }
    return conn;
}

bool GenUuid(std::string& value, std::string& error)
{
    return memochat::random::GenerateUuid(value, &error);
}

} // namespace

class AISessionRepo::Impl
{
public:
    std::unique_ptr<pqxx::connection> conn;
    std::mutex mutex;
    std::string init_error;

    Impl()
    {
        conn = GetPgConn(init_error);
    }

    bool Ready() const
    {
        return conn != nullptr && conn->is_open();
    }
};

AISessionRepo::AISessionRepo()
    : _impl(std::make_unique<Impl>())
{
}

AISessionRepo::~AISessionRepo() = default;

AISessionCreateResult AISessionRepo::Create(int32_t uid, const std::string& model_type, const std::string& model_name)
{
    std::string session_id;
    std::string uuid_error;
    if (!GenUuid(session_id, uuid_error))
    {
        memolog::LogError("ai_session.create.failed",
                          "uuid_generation_failed",
                          {{"uid", std::to_string(uid)}, {"error", uuid_error}});
        return std::unexpected(AISessionCreateError::UuidGenerationFailed);
    }
    int64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    if (!_impl->Ready())
    {
        memolog::LogError("ai_session.create.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"error", _impl->init_error}});
        return std::unexpected(AISessionCreateError::StorageUnavailable);
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto inserted = tx.exec_params(
        R"(INSERT INTO ai_session (session_id, uid, title, model_type, model_name, created_at, updated_at)
           VALUES ($1, $2, $3, $4, $5, $6, $7)
           ON CONFLICT (session_id) DO NOTHING)",
        session_id,
        uid,
        repository_modules::DefaultSessionTitle(),
        model_type,
        model_name,
        now,
        now);
    if (!inserted.ok() ||
        !repository_modules::HasAffectedRows(static_cast<unsigned long long>(inserted.affected_rows())) || !tx.commit())
    {
        memolog::LogError(
            "ai_session.create.failed",
            "pg_insert_error",
            {{"uid", std::to_string(uid)}, {"error", inserted.ok() ? tx.error_message() : inserted.error_message()}});
        return std::unexpected(AISessionCreateError::InsertFailed);
    }
    return session_id;
}

bool AISessionRepo::SoftDelete(int32_t uid, const std::string& session_id)
{
    int64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (!_impl->Ready())
    {
        memolog::LogError("ai_session.delete.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", _impl->init_error}});
        return false;
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto result = tx.exec_params(R"(UPDATE ai_session
                                           SET deleted_at = $1
                                           WHERE uid = $2 AND session_id = $3 AND deleted_at IS NULL)",
                                       now,
                                       uid,
                                       session_id);
    const bool affected = repository_modules::HasAffectedRows(static_cast<unsigned long long>(result.affected_rows()));
    if (!result.ok() || !tx.commit())
    {
        memolog::LogError("ai_session.delete.failed",
                          "pg_update_error",
                          {{"uid", std::to_string(uid)},
                           {"session_id", session_id},
                           {"error", result.ok() ? tx.error_message() : result.error_message()}});
        return false;
    }
    return affected;
}

bool AISessionRepo::UpdateTitle(int32_t uid, const std::string& session_id, const std::string& title)
{
    int64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (!_impl->Ready())
    {
        memolog::LogError("ai_session.update_title.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", _impl->init_error}});
        return false;
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto result = tx.exec_params(
        R"(UPDATE ai_session
           SET title = $1, updated_at = $2
           WHERE uid = $3 AND session_id = $4 AND deleted_at IS NULL)",
        title,
        now,
        uid,
        session_id);
    const bool affected = repository_modules::HasAffectedRows(static_cast<unsigned long long>(result.affected_rows()));
    if (!result.ok() || !tx.commit())
    {
        memolog::LogError("ai_session.update_title.failed",
                          "pg_update_error",
                          {{"uid", std::to_string(uid)},
                           {"session_id", session_id},
                           {"error", result.ok() ? tx.error_message() : result.error_message()}});
        return false;
    }
    return affected;
}

std::unique_ptr<ai::AISessionInfo> AISessionRepo::GetSession(int32_t uid, const std::string& session_id)
{
    if (!_impl->Ready())
    {
        memolog::LogError("ai_session.get.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", _impl->init_error}});
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto result = tx.exec_params(
        R"(SELECT session_id, title, model_type, model_name, created_at, updated_at
           FROM ai_session WHERE uid = $1 AND session_id = $2 AND deleted_at IS NULL)",
        uid,
        session_id);
    if (!result.ok() || repository_modules::ShouldReturnNoSession(result.empty()))
    {
        if (!result.ok())
        {
            memolog::LogError(
                "ai_session.get.failed",
                "pg_select_error",
                {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", result.error_message()}});
        }
        return nullptr;
    }

    const auto row = result[0];
    auto info = std::make_unique<ai::AISessionInfo>();
    info->set_session_id(row["session_id"].as<std::string>());
    info->set_title(row["title"].as<std::string>());
    info->set_model_type(row["model_type"].as<std::string>());
    info->set_model_name(row["model_name"].as<std::string>());
    info->set_created_at(row["created_at"].as<int64_t>());
    info->set_updated_at(row["updated_at"].as<int64_t>());
    if (!result.ok() || !tx.commit())
    {
        memolog::LogError("ai_session.get.failed",
                          "pg_select_error",
                          {{"uid", std::to_string(uid)},
                           {"session_id", session_id},
                           {"error", result.ok() ? tx.error_message() : result.error_message()}});
        return nullptr;
    }
    return info;
}

std::vector<ai::AISessionInfo> AISessionRepo::ListByUid(int32_t uid)
{
    std::vector<ai::AISessionInfo> result;
    if (!_impl->Ready())
    {
        memolog::LogError("ai_session.list.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"error", _impl->init_error}});
        return result;
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto rows = tx.exec_params(
        R"(SELECT session_id, title, model_type, model_name, created_at, updated_at
           FROM ai_session WHERE uid = $1 AND deleted_at IS NULL
           ORDER BY updated_at DESC)",
        uid);
    if (!rows.ok())
    {
        memolog::LogError("ai_session.list.failed",
                          "pg_select_error",
                          {{"uid", std::to_string(uid)}, {"error", rows.error_message()}});
        return result;
    }
    for (const auto& row : rows)
    {
        ai::AISessionInfo info;
        info.set_session_id(row["session_id"].as<std::string>());
        info.set_title(row["title"].as<std::string>());
        info.set_model_type(row["model_type"].as<std::string>());
        info.set_model_name(row["model_name"].as<std::string>());
        info.set_created_at(row["created_at"].as<int64_t>());
        info.set_updated_at(row["updated_at"].as<int64_t>());
        result.push_back(std::move(info));
    }
    if (!rows.ok() || !tx.commit())
    {
        memolog::LogError(
            "ai_session.list.failed",
            "pg_select_error",
            {{"uid", std::to_string(uid)}, {"error", rows.ok() ? tx.error_message() : rows.error_message()}});
        result.clear();
    }
    return result;
}

bool AISessionRepo::SaveMessage(const std::string& session_id,
                                int32_t uid,
                                const std::string& role,
                                const std::string& content,
                                const std::string& model_name,
                                int64_t tokens_used)
{
    std::string msg_id;
    std::string uuid_error;
    if (!GenUuid(msg_id, uuid_error))
    {
        memolog::LogError("ai_message.save.failed",
                          "uuid_generation_failed",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", uuid_error}});
        return false;
    }
    int64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    if (!_impl->Ready())
    {
        memolog::LogError("ai_message.save.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", _impl->init_error}});
        return false;
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto inserted = tx.exec_params(
        R"(INSERT INTO ai_message (msg_id, session_id, role, content, tokens_used, model_name, created_at)
           SELECT $1, s.session_id, $3, $4, $5, $6, $7
           FROM ai_session s
           WHERE s.session_id = $2 AND s.uid = $8 AND s.deleted_at IS NULL)",
        msg_id,
        session_id,
        role,
        content,
        tokens_used,
        model_name,
        now,
        uid);
    if (!inserted.ok() ||
        !repository_modules::HasAffectedRows(static_cast<unsigned long long>(inserted.affected_rows())))
    {
        if (!inserted.ok())
        {
            memolog::LogError(
                "ai_message.save.failed",
                "pg_insert_error",
                {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", inserted.error_message()}});
        }
        return false;
    }

    const auto updated = tx.exec_params(R"(UPDATE ai_session SET updated_at = $1 WHERE uid = $2 AND session_id = $3)",
                                        now,
                                        uid,
                                        session_id);
    if (!updated.ok() || !tx.commit())
    {
        memolog::LogError("ai_message.save.failed",
                          "pg_insert_error",
                          {{"uid", std::to_string(uid)},
                           {"session_id", session_id},
                           {"error", updated.ok() ? tx.error_message() : updated.error_message()}});
        return false;
    }
    return true;
}

std::vector<ai::AIMessage> AISessionRepo::GetMessages(int32_t uid, const std::string& session_id, int limit, int offset)
{
    std::vector<ai::AIMessage> result;
    if (!_impl->Ready())
    {
        memolog::LogError("ai_message.get.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", _impl->init_error}});
        return result;
    }
    std::lock_guard<std::mutex> lock(_impl->mutex);
    pqxx::work tx(*_impl->conn);
    const auto rows = tx.exec_params(
        R"(SELECT m.msg_id, m.role, m.content, m.created_at
           FROM ai_message m
           JOIN ai_session s ON s.session_id = m.session_id
           WHERE s.uid = $1 AND m.session_id = $2 AND s.deleted_at IS NULL AND m.deleted_at IS NULL
           ORDER BY m.created_at ASC LIMIT $3 OFFSET $4)",
        uid,
        session_id,
        limit,
        offset);
    if (!rows.ok())
    {
        memolog::LogError("ai_message.get.failed",
                          "pg_select_error",
                          {{"uid", std::to_string(uid)}, {"session_id", session_id}, {"error", rows.error_message()}});
        return result;
    }
    for (const auto& row : rows)
    {
        ai::AIMessage message;
        message.set_msg_id(row["msg_id"].as<std::string>());
        message.set_role(row["role"].as<std::string>());
        message.set_content(row["content"].as<std::string>());
        message.set_created_at(row["created_at"].as<int64_t>());
        result.push_back(std::move(message));
    }
    if (!rows.ok() || !tx.commit())
    {
        memolog::LogError("ai_message.get.failed",
                          "pg_select_error",
                          {{"uid", std::to_string(uid)},
                           {"session_id", session_id},
                           {"error", rows.ok() ? tx.error_message() : rows.error_message()}});
        result.clear();
    }
    return result;
}
