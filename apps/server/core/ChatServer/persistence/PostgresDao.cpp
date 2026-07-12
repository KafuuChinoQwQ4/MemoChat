#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include "SnowflakeUtil.hpp"
#include <set>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <limits>
#include <sstream>

import memochat.chat.postgres_dao_algorithms;

namespace postgres_dao_modules = memochat::chat::persistence::postgres_dao::modules;

namespace
{
std::string BuildPostgresConnectionStringFor(const std::string& section, const std::string& fallback_section)
{
    auto& cfg = ConfigMgr::Inst();
    std::string sec = section;
    if (postgres_dao_modules::ShouldUseFallbackSection(cfg[section]["Host"].empty(), fallback_section.empty()))
    {
        sec = fallback_section; // section absent -> reuse fallback (behavior preserved)
    }
    const auto host = cfg[sec]["Host"];
    if (!postgres_dao_modules::HasPostgresHost(host.empty()))
    {
        return "";
    }
    const auto port = cfg[sec]["Port"];
    const auto pwd = cfg[sec]["Passwd"];
    const auto database = cfg[sec]["Database"];
    const auto schema = cfg[sec]["Schema"];
    const auto user = cfg[sec]["User"];
    const auto sslmode = cfg[sec]["SslMode"];
    const auto* selected_sslmode = postgres_dao_modules::SelectSslMode(sslmode.empty(), sslmode.c_str());
    const auto* selected_schema = postgres_dao_modules::SelectSchema(schema.empty(), schema.c_str());
    return "host=" + host + " port=" + port + " user=" + user + " password=" + pwd + " dbname=" + database +
           " sslmode=" + selected_sslmode + " options=-csearch_path=" + selected_schema + ",public";
}

std::string BuildPostgresConnectionString()
{
    return BuildPostgresConnectionStringFor("Postgres", "");
}
} // namespace
PostgresDao::PostgresDao()
{
    postgres_connection_string_ = BuildPostgresConnectionString();
    // Account-data (user/user_id) connection. Defaults to the same [Postgres]
    // database when [AccountPostgres] is absent, so behavior is unchanged until
    // the memo_account split config is set (gateserver split Phase 2b). When set,
    // only the user/user_id queries follow this string; chat tables stay on
    // [Postgres]. This is the DB-per-service seam for the account aggregate.
    account_connection_string_ = BuildPostgresConnectionStringFor("AccountPostgres", "Postgres");
    if (postgres_connection_string_.empty() || account_connection_string_.empty())
    {
        startup_error_ = "missing PostgreSQL configuration for ChatServer";
        std::cerr << startup_error_ << std::endl;
        return;
    }

    pqxx::connection chat_connection(postgres_connection_string_);
    if (!chat_connection.is_open())
    {
        startup_error_ = "ChatServer PostgreSQL connection failed: " + chat_connection.error_message();
        std::cerr << startup_error_ << std::endl;
        return;
    }
    pqxx::connection account_connection(account_connection_string_);
    if (!account_connection.is_open())
    {
        startup_error_ = "ChatServer account PostgreSQL connection failed: " + account_connection.error_message();
        std::cerr << startup_error_ << std::endl;
        return;
    }
    {
        pqxx::read_transaction account_schema_transaction(account_connection);
        const auto account_schema = account_schema_transaction.exec(
            "SELECT uid, name, email, pwd, password_hash, user_id, nick, icon, \"desc\", sex "
            "FROM \"user\" WHERE FALSE");
        if (!account_schema.ok())
        {
            startup_error_ =
                "ChatServer account PostgreSQL schema validation failed: " + account_schema.error_message();
            std::cerr << startup_error_ << std::endl;
            return;
        }
        const auto user_id_schema = account_schema_transaction.exec("SELECT id FROM user_id WHERE FALSE");
        if (!user_id_schema.ok())
        {
            startup_error_ =
                "ChatServer account PostgreSQL user_id schema validation failed: " + user_id_schema.error_message();
            std::cerr << startup_error_ << std::endl;
            return;
        }
    }

    if (!EnsureChatMessageIdempotencySchema())
    {
        startup_error_ = "ChatServer PostgreSQL idempotency schema initialization failed";
        return;
    }
    if (!EnsureChatEventOutboxSchema())
    {
        startup_error_ = "ChatServer PostgreSQL outbox schema initialization failed";
        return;
    }
    if (!WarmupRelationBootstrapQueries())
    {
        if (startup_error_.empty())
        {
            startup_error_ = "ChatServer PostgreSQL relation query warmup failed";
        }
        return;
    }
    ready_ = true;
}

PostgresDao::~PostgresDao() = default;

bool PostgresDao::Ready() const noexcept
{
    return ready_;
}

const std::string& PostgresDao::startupError() const noexcept
{
    return startup_error_;
}

bool PostgresDao::WarmupRelationBootstrapQueries()
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        startup_error_ = "warmup relation bootstrap queries failed: " + postgres_error;
        std::cerr << "warmup relation bootstrap queries failed: " << postgres_error << std::endl;
        return false;
    }
    // Warm only relation tables; user base-info is resolved separately
    // via GetUsersByUids (account-data seam), so no JOIN "user" here —
    // keeps the warmup valid after the user table moves to memo_account.
    txn.exec_params("SELECT a.from_uid, a.status "
                    "FROM friend_apply AS a "
                    "WHERE a.to_uid = $1 AND a.id > $2 ORDER BY a.id ASC LIMIT $3",
                    -1,
                    0,
                    1);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        startup_error_ = "warmup relation bootstrap queries failed: " + postgres_error;
        std::cerr << "warmup relation bootstrap queries failed: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("SELECT tag FROM friend_apply_tag WHERE to_uid = $1 AND from_uid = $2 ORDER BY id ASC", -1, -1);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        startup_error_ = "warmup relation bootstrap queries failed: " + postgres_error;
        std::cerr << "warmup relation bootstrap queries failed: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("SELECT f.friend_id, f.back "
                    "FROM friend AS f "
                    "WHERE f.self_id = $1 LIMIT 1",
                    -1);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        startup_error_ = "warmup relation bootstrap queries failed: " + postgres_error;
        std::cerr << "warmup relation bootstrap queries failed: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("SELECT tag FROM friend_tag WHERE self_id = $1 AND friend_id = $2 ORDER BY id ASC", -1, -1);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        startup_error_ = "warmup relation bootstrap queries failed: " + postgres_error;
        std::cerr << "warmup relation bootstrap queries failed: " << postgres_error << std::endl;
        return false;
    }
    return true;
}
