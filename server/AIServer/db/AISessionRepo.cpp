#include "db/AISessionRepo.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include <libpqxx/pqxx>
#include <libpqxx/transaction.hxx>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <sstream>

using namespace std::chrono;

namespace {

pqxx::connection* GetPgConn() {
    auto& cfg = ConfigMgr::Inst();
    std::ostringstream conn_str;
    conn_str << "host=" << cfg["Postgres"]["Host"]
             << " port=" << cfg["Postgres"]["Port"]
             << " user=" << cfg["Postgres"]["User"]
             << " password=" << cfg["Postgres"]["Passwd"]
             << " dbname=" << cfg["Postgres"]["Database"]
             << " options=-c search_path=" << cfg["Postgres"]["Schema"];
    return new pqxx::connection(conn_str.str());
}

std::string GenUuid() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

} // namespace

class AISessionRepo::Impl {
public:
    std::unique_ptr<pqxx::connection> conn;

    Impl() {
        conn.reset(GetPgConn());
    }
};

AISessionRepo::AISessionRepo()
    : _impl(std::make_unique<Impl>()) {}

AISessionRepo::~AISessionRepo() = default;

std::string AISessionRepo::Create(int32_t uid, const std::string& model_type,
                                  const std::string& model_name) {
    std::string session_id = GenUuid();
    int64_t now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();

    try {
        pqxx::work tx(*_impl->conn);
        tx.exec_params(
            R"(INSERT INTO ai_session (session_id, uid, title, model_type, model_name, created_at, updated_at)
               VALUES ($1, $2, $3, $4, $5, $6, $7)
               ON CONFLICT (session_id) DO NOTHING)",
            session_id, uid, "", model_type, model_name, now, now);
        tx.commit();
    } catch (const std::exception& e) {
        memolog::LogError("ai_session.create.failed",
            "pg_insert_error",
            {{"uid", std::to_string(uid)}, {"error", e.what()}});
    }
    return session_id;
}

bool AISessionRepo::SoftDelete(const std::string& session_id) {
    int64_t now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
    try {
        pqxx::work tx(*_impl->conn);
        auto r = tx.exec_params(
            R"(UPDATE ai_session SET deleted_at = $1 WHERE session_id = $2 AND deleted_at IS NULL)",
            now, session_id);
        tx.commit();
        return r.affected_rows() > 0;
    } catch (const std::exception& e) {
        memolog::LogError("ai_session.delete.failed",
            "pg_update_error",
            {{"session_id", session_id}, {"error", e.what()}});
        return false;
    }
}

std::unique_ptr<ai::AISessionInfo> AISessionRepo::GetSession(const std::string& session_id) {
    try {
        pqxx::work tx(*_impl->conn);
        auto r = tx.exec_params(
            R"(SELECT session_id, title, model_type, model_name, created_at, updated_at
               FROM ai_session WHERE session_id = $1 AND deleted_at IS NULL)",
            session_id);
        tx.commit();
        if (r.empty()) return nullptr;

        const auto& row = r[0];
        auto info = std::make_unique<ai::AISessionInfo>();
        info->set_session_id(row["session_id"].as<std::string>());
        info->set_title(row["title"].as<std::string>());
        info->set_model_type(row["model_type"].as<std::string>());
        info->set_model_name(row["model_name"].as<std::string>());
        info->set_created_at(row["created_at"].as<int64_t>());
        info->set_updated_at(row["updated_at"].as<int64_t>());
        return info;
    } catch (const std::exception& e) {
        memolog::LogError("ai_session.get.failed",
            "pg_select_error",
            {{"session_id", session_id}, {"error", e.what()}});
        return nullptr;
    }
}

std::vector<ai::AISessionInfo> AISessionRepo::ListByUid(int32_t uid) {
    std::vector<ai::AISessionInfo> result;
    try {
        pqxx::work tx(*_impl->conn);
        auto r = tx.exec_params(
            R"(SELECT session_id, title, model_type, model_name, created_at, updated_at
               FROM ai_session WHERE uid = $1 AND deleted_at IS NULL
               ORDER BY updated_at DESC)",
            uid);
        tx.commit();
        for (const auto& row : r) {
            ai::AISessionInfo info;
            info.set_session_id(row["session_id"].as<std::string>());
            info.set_title(row["title"].as<std::string>());
            info.set_model_type(row["model_type"].as<std::string>());
            info.set_model_name(row["model_name"].as<std::string>());
            info.set_created_at(row["created_at"].as<int64_t>());
            info.set_updated_at(row["updated_at"].as<int64_t>());
            result.push_back(std::move(info));
        }
    } catch (const std::exception& e) {
        memolog::LogError("ai_session.list.failed",
            "pg_select_error",
            {{"uid", std::to_string(uid)}, {"error", e.what()}});
    }
    return result;
}

bool AISessionRepo::SaveMessage(const std::string& session_id, int32_t uid,
                                 const std::string& role,
                                 const std::string& content,
                                 const std::string& model_name,
                                 int64_t tokens_used) {
    std::string msg_id = GenUuid();
    int64_t now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();

    try {
        pqxx::work tx(*_impl->conn);
        tx.exec_params(
            R"(INSERT INTO ai_message (msg_id, session_id, role, content, tokens_used, model_name, created_at)
               VALUES ($1, $2, $3, $4, $5, $6, $7))",
            msg_id, session_id, role, content, tokens_used, model_name, now);

        tx.exec_params(
            R"(UPDATE ai_session SET updated_at = $1 WHERE session_id = $2)",
            now, session_id);
        tx.commit();
        return true;
    } catch (const std::exception& e) {
        memolog::LogError("ai_message.save.failed",
            "pg_insert_error",
            {{"session_id", session_id}, {"error", e.what()}});
        return false;
    }
}

std::vector<ai::AIMessage> AISessionRepo::GetMessages(const std::string& session_id,
                                                      int limit, int offset) {
    std::vector<ai::AIMessage> result;
    try {
        pqxx::work tx(*_impl->conn);
        auto r = tx.exec_params(
            R"(SELECT msg_id, role, content, created_at
               FROM ai_message WHERE session_id = $1 AND deleted_at IS NULL
               ORDER BY created_at ASC LIMIT $2 OFFSET $3)",
            session_id, limit, offset);
        tx.commit();
        for (const auto& row : r) {
            ai::AIMessage m;
            m.set_msg_id(row["msg_id"].as<std::string>());
            m.set_role(row["role"].as<std::string>());
            m.set_content(row["content"].as<std::string>());
            m.set_created_at(row["created_at"].as<int64_t>());
            result.push_back(std::move(m));
        }
    } catch (const std::exception& e) {
        memolog::LogError("ai_message.get.failed",
            "pg_select_error",
            {{"session_id", session_id}, {"error", e.what()}});
    }
    return result;
}
