#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include "PostgresPool.hpp"
#include "SnowflakeUtil.hpp"
#include <pqxx/pqxx>
#include <set>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <stdexcept>
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
    use_postgres_ = postgres_dao_modules::ShouldEnablePostgres(postgres_connection_string_.empty());
    if (!use_postgres_)
    {
        throw std::runtime_error("missing [Postgres] configuration for ChatServer");
    }
    auto* raw_pool = new PostgresPool(postgres_connection_string_, "", "", "", postgres_dao_modules::StartupPoolSize());
    pool_.reset(raw_pool);
    EnsureChatMessageIdempotencySchema();
    EnsureChatEventOutboxSchema();
    WarmupRelationBootstrapQueries();
}

PostgresDao::~PostgresDao()
{
    if (pool_)
    {
        pool_->Close();
    }
}

void PostgresDao::WarmupRelationBootstrapQueries()
{
    if (postgres_dao_modules::ShouldUsePostgresWarmupPath(use_postgres_))
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            // Warm only relation tables; user base-info is resolved separately
            // via GetUsersByUids (account-data seam), so no JOIN "user" here —
            // keeps the warmup valid after the user table moves to memo_account.
            txn.exec_params("SELECT a.from_uid, a.status "
                            "FROM friend_apply AS a "
                            "WHERE a.to_uid = $1 AND a.id > $2 ORDER BY a.id ASC LIMIT $3",
                            -1,
                            0,
                            1);
            txn.exec_params("SELECT tag FROM friend_apply_tag WHERE to_uid = $1 AND from_uid = $2 ORDER BY id ASC",
                            -1,
                            -1);
            txn.exec_params("SELECT f.friend_id, f.back "
                            "FROM friend AS f "
                            "WHERE f.self_id = $1 LIMIT 1",
                            -1);
            txn.exec_params("SELECT tag FROM friend_tag WHERE self_id = $1 AND friend_id = $2 ORDER BY id ASC", -1, -1);
            return;
        }
        catch (const std::exception& e)
        {
            std::cerr << "warmup relation bootstrap queries failed: " << e.what() << std::endl;
            return;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "select apply.from_uid, apply.status, user.name, user.nick, user.sex, user.user_id, user.icon "
                "from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
                "and apply.id > ? order by apply.id ASC LIMIT ? "));
            pstmt->setInt(1, -1);
            pstmt->setInt(2, 0);
            pstmt->setInt(3, 1);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            (void) res;
        }

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "SELECT from_uid, tag FROM friend_apply_tag WHERE to_uid = ? AND from_uid IN (?) ORDER BY id ASC"));
            pstmt->setInt(1, -1);
            pstmt->setInt(2, -1);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            (void) res;
        }

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "SELECT f.friend_id, f.back, u.name, u.nick, u.sex, u.user_id, u.`desc`, u.icon "
                "FROM friend AS f "
                "JOIN user AS u ON f.friend_id = u.uid "
                "WHERE f.self_id = ? LIMIT 1"));
            pstmt->setInt(1, -1);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            (void) res;
        }

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "SELECT friend_id, tag FROM friend_tag WHERE self_id = ? AND friend_id IN (?) ORDER BY id ASC"));
            pstmt->setInt(1, -1);
            pstmt->setInt(2, -1);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            (void) res;
        }
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "warmup relation bootstrap queries failed: " << e.what() << std::endl;
    }
}
