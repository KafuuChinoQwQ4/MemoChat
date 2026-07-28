#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include "SnowflakeUtil.hpp"
#include <set>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <limits>
#include <sstream>
#include <string_view>
#include <thread>
#include <utility>

#ifndef _WIN32
#include <poll.h>
#endif

import memochat.chat.postgres_dao_algorithms;

namespace postgres_dao_modules = memochat::chat::persistence::postgres_dao::modules;

namespace
{
enum class PostgresConnectionPurpose
{
    Application,
    HealthProbe,
};

std::string BuildPostgresConnectionStringFor(const std::string& section,
                                             const std::string& fallback_section,
                                             PostgresConnectionPurpose purpose = PostgresConnectionPurpose::Application)
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
    std::string connection_string = "host=" + host + " port=" + port + " user=" + user + " password=" + pwd +
                                    " dbname=" + database + " sslmode=" + selected_sslmode +
                                    " connect_timeout=" + std::to_string(postgres_dao_modules::ConnectTimeoutSeconds());
    if (purpose == PostgresConnectionPurpose::HealthProbe)
    {
        return connection_string +
               " tcp_user_timeout=" + std::to_string(postgres_dao_modules::TcpUserTimeoutMilliseconds()) +
               " options='" + postgres_dao_modules::HealthProbeSessionOptions() + "'";
    }
    return connection_string + " options=-csearch_path=" + selected_schema + ",public";
}

std::string BuildPostgresConnectionString()
{
    return BuildPostgresConnectionStringFor("Postgres", "");
}

struct PgConnectionDeleter
{
    void operator()(PGconn* connection) const noexcept
    {
        if (connection != nullptr)
        {
            PQfinish(connection);
        }
    }
};

struct PgResultDeleter
{
    void operator()(PGresult* result) const noexcept
    {
        if (result != nullptr)
        {
            PQclear(result);
        }
    }
};

using PgConnection = std::unique_ptr<PGconn, PgConnectionDeleter>;
using PgResult = std::unique_ptr<PGresult, PgResultDeleter>;

struct SocketReadiness
{
    bool ready = false;
    bool readable = false;
    bool writable = false;
};

SocketReadiness WaitForPostgresSocket(PGconn* connection,
                                      bool want_read,
                                      bool want_write,
                                      std::chrono::steady_clock::time_point deadline) noexcept
{
    const int socket = PQsocket(connection);
    if (socket < 0)
    {
        return {};
    }

#ifdef _WIN32
    if (std::chrono::steady_clock::now() >= deadline)
    {
        return {};
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return SocketReadiness{.ready = true, .readable = want_read, .writable = want_write};
#else
    while (true)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            return {};
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        const int timeout_milliseconds = static_cast<int>(std::max<int64_t>(1, remaining.count()));
        pollfd descriptor{.fd = socket,
                          .events = static_cast<short>((want_read ? POLLIN : 0) | (want_write ? POLLOUT : 0)),
                          .revents = 0};
        const int result = ::poll(&descriptor, 1, timeout_milliseconds);
        if (result == 0)
        {
            return {};
        }
        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return {};
        }
        if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0)
        {
            return {};
        }
        return SocketReadiness{.ready = true,
                               .readable = (descriptor.revents & (POLLIN | POLLHUP)) != 0,
                               .writable = (descriptor.revents & POLLOUT) != 0};
    }
#endif
}

PgConnection ConnectPostgresForProbe(std::string_view connection_string) noexcept
{
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(postgres_dao_modules::ConnectTimeoutSeconds());
    const std::string owned_connection_string(connection_string);
    PgConnection connection(PQconnectStart(owned_connection_string.c_str()));
    if (!connection || PQstatus(connection.get()) == CONNECTION_BAD)
    {
        return {};
    }

    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto state = PQconnectPoll(connection.get());
        if (state == PGRES_POLLING_OK)
        {
            if (PQsetnonblocking(connection.get(), 1) != 0)
            {
                return {};
            }
            return connection;
        }
        if (state == PGRES_POLLING_FAILED)
        {
            return {};
        }
        if (state == PGRES_POLLING_ACTIVE)
        {
            continue;
        }

        const bool want_read = state == PGRES_POLLING_READING;
        const bool want_write = state == PGRES_POLLING_WRITING;
        if (!WaitForPostgresSocket(connection.get(), want_read, want_write, deadline).ready)
        {
            return {};
        }
    }
    return {};
}

bool RunPostgresProbeQuery(PGconn* connection) noexcept
{
    if (PQsendQuery(connection, "SELECT 1") != 1)
    {
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(postgres_dao_modules::HealthProbeDeadlineMilliseconds());
    while (std::chrono::steady_clock::now() < deadline)
    {
        const int flush = PQflush(connection);
        if (flush < 0)
        {
            return false;
        }
        if (flush == 0 && PQisBusy(connection) == 0)
        {
            break;
        }

        const auto readiness = WaitForPostgresSocket(connection, true, flush == 1, deadline);
        if (!readiness.ready)
        {
            return false;
        }
        if (readiness.readable && PQconsumeInput(connection) != 1)
        {
            return false;
        }
    }
    bool saw_result = false;
    bool healthy = false;
    while (true)
    {
        while (PQisBusy(connection) != 0)
        {
            const auto readiness = WaitForPostgresSocket(connection, true, false, deadline);
            if (!readiness.ready || !readiness.readable || PQconsumeInput(connection) != 1)
            {
                return false;
            }
        }

        PGresult* raw_result = PQgetResult(connection);
        if (raw_result == nullptr)
        {
            return saw_result && healthy;
        }
        PgResult result(raw_result);
        const bool current_healthy = PQresultStatus(result.get()) == PGRES_TUPLES_OK && PQntuples(result.get()) == 1 &&
                                     PQnfields(result.get()) == 1 && PQgetisnull(result.get(), 0, 0) == 0 &&
                                     std::strcmp(PQgetvalue(result.get(), 0, 0), "1") == 0;
        healthy = !saw_result && current_healthy;
        saw_result = true;
    }
}

bool ProbePostgresConnection(void*, std::string_view connection_string) noexcept
{
    auto connection = ConnectPostgresForProbe(connection_string);
    return connection && RunPostgresProbeQuery(connection.get());
}
} // namespace
PostgresDao::PostgresDao()
{
    postgres_connection_string_ = BuildPostgresConnectionString();
    auto postgres_health_connection_string =
        BuildPostgresConnectionStringFor("Postgres", "", PostgresConnectionPurpose::HealthProbe);
    // Account-data (user/user_id) connection. Defaults to the same [Postgres]
    // database when [AccountPostgres] is absent, so behavior is unchanged until
    // the memo_account split config is set (gateserver split Phase 2b). When set,
    // only the user/user_id queries follow this string; chat tables stay on
    // [Postgres]. This is the DB-per-service seam for the account aggregate.
    account_connection_string_ = BuildPostgresConnectionStringFor("AccountPostgres", "Postgres");
    auto account_health_connection_string =
        BuildPostgresConnectionStringFor("AccountPostgres", "Postgres", PostgresConnectionPurpose::HealthProbe);
    if (postgres_connection_string_.empty() || account_connection_string_.empty() ||
        postgres_health_connection_string.empty() || account_health_connection_string.empty())
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

    if (!ValidateChatMessageIdempotencySchema())
    {
        startup_error_ = "ChatServer PostgreSQL idempotency schema validation failed";
        return;
    }
    if (!ValidateChatEventOutboxSchema())
    {
        startup_error_ = "ChatServer PostgreSQL outbox schema validation failed";
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

    std::string monitor_error;
    if (!health_monitor_.Start(std::move(postgres_health_connection_string),
                               std::move(account_health_connection_string),
                               memochat::chat::persistence::PostgresHealthProbe{.function = &ProbePostgresConnection},
                               std::chrono::milliseconds(postgres_dao_modules::HealthProbeIntervalMilliseconds()),
                               true,
                               &monitor_error))
    {
        startup_error_ = monitor_error;
        std::cerr << startup_error_ << std::endl;
        return;
    }
    ready_ = true;
}

PostgresDao::~PostgresDao()
{
    std::string monitor_error;
    if (!health_monitor_.Stop(&monitor_error))
    {
        std::cerr << "PostgreSQL health monitor join failed: " << monitor_error << std::endl;
    }
}

bool PostgresDao::Ready() const noexcept
{
    return ready_;
}

bool PostgresDao::CheckHealth() const noexcept
{
    return health_monitor_.Healthy();
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
