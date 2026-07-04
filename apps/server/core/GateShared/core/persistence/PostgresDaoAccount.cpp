#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "SnowflakeUtil.hpp"
#include "auth/PasswordHasher.hpp"
#include "auth/RefreshToken.hpp"
#include "db/PqxxCompat.hpp"
#include <string_view>

import memochat.gate.postgres_dao_account_algorithms;

namespace postgres_dao_account_modules = memochat::gate::postgres_dao_account::modules;

namespace
{
std::string PostgresSchemaName()
{
    auto& cfg = ConfigMgr::Inst();
    const auto schema = cfg["Postgres"]["Schema"];
    return schema.empty() ? postgres_dao_account_modules::DefaultSchema() : schema;
}

std::string QuotePgIdent(std::string_view ident)
{
    std::string out = "\"";
    for (char ch : ident)
    {
        if (ch == '"')
        {
            out += "\"\"";
        }
        else
        {
            out += ch;
        }
    }
    out += "\"";
    return out;
}

std::string PgTable(std::string_view table)
{
    return QuotePgIdent(PostgresSchemaName()) + "." + QuotePgIdent(table);
}

std::string ClampText(std::string value, std::size_t max_size)
{
    if (value.size() > max_size)
    {
        value.resize(max_size);
    }
    return value;
}
} // namespace

int PostgresDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return RegUserTransaction(name, email, pwd, postgres_dao_account_modules::DefaultUserIcon());
}

int PostgresDao::RegUserTransaction(const std::string& name,
                                    const std::string& email,
                                    const std::string& pwd,
                                    const std::string& icon)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return -1;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::string password_hash;
        if (!memochat::auth::HashPassword(pwd, password_hash))
        {
            return -1;
        }

        pqxx::work txn(*con->_con);
        const auto exists = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
        if (!exists.empty())
        {
            txn.commit();
            std::cout << "email " << email << " exists" << std::endl;
            return 0;
        }

        const int new_id = txn.exec1("SELECT COALESCE(MAX(id), 1000) + 1 FROM user_id")[0].as<int>();
        txn.exec0("DELETE FROM user_id");
        txn.exec_params0("INSERT INTO user_id(id) VALUES ($1)", new_id);

        std::string user_public_id;
        for (int i = 0; i < postgres_dao_account_modules::UserPublicIdMaxAttempts(); ++i)
        {
            user_public_id = GenerateUserPublicId();
            const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE user_id = $1 LIMIT 1", user_public_id);
            if (postgres_dao_account_modules::ShouldAcceptGeneratedPublicId(rows.empty()))
            {
                break;
            }
            user_public_id.clear();
        }

        if (!postgres_dao_account_modules::HasGeneratedUserPublicId(user_public_id.empty()))
        {
            std::cout << "generate user_public_id failed" << std::endl;
            return -1;
        }

        txn.exec_params0("INSERT INTO \"user\"(uid, name, email, pwd, password_hash, nick, icon, user_id) "
                         "VALUES ($1, $2, $3, '', $4, $5, $6, $7)",
                         new_id,
                         name,
                         email,
                         password_hash,
                         name,
                         icon,
                         user_public_id);
        txn.commit();
        std::cout << "newuser insert into user success" << std::endl;
        return new_id;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return -1;
    }
}

bool PostgresDao::CheckEmail(const std::string& name, const std::string& email)
{
    (void) name;
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
        pqxx::read_transaction txn(*con->_con);
        const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
        return !rows.empty();
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::UpdatePwd(const std::string& email, const std::string& newpwd)
{
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
        std::string password_hash;
        if (!memochat::auth::HashPassword(newpwd, password_hash))
        {
            return false;
        }

        pqxx::work txn(*con->_con);
        const auto result = txn.exec_params("UPDATE \"user\" SET password_hash = $1, pwd = '' WHERE email = $2 "
                                            "RETURNING uid",
                                            password_hash,
                                            email);
        if (!result.empty())
        {
            const int uid = result[0]["uid"].as<int>();
            txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                                 " SET revoked_at = now() WHERE uid = $1 AND revoked_at IS NULL",
                             uid);
        }
        txn.commit();
        std::cout << "Updated rows: " << result.affected_rows() << std::endl;
        return postgres_dao_account_modules::IsAffectedRowsPositive(result.affected_rows());
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::IssueRefreshToken(int uid,
                                    const std::string& selector,
                                    const std::string& verifier_hash,
                                    int ttl_seconds,
                                    const std::string& user_agent,
                                    const std::string& ip_hash)
{
    if (uid <= 0 || selector.empty() || !memochat::auth::LooksLikeRefreshTokenHash(verifier_hash) || ttl_seconds <= 0)
    {
        return false;
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
        pqxx::work txn(*con->_con);
        const auto result =
            txn.exec_params("INSERT INTO " + PgTable("auth_refresh_token") +
                                "(uid, selector, verifier_hash, expires_at, user_agent, ip_hash) "
                                "VALUES ($1, $2, $3, now() + ($4::integer * interval '1 second'), $5, $6) "
                                "ON CONFLICT (selector) DO NOTHING",
                            uid,
                            selector,
                            verifier_hash,
                            ttl_seconds,
                            ClampText(user_agent, 512),
                            ip_hash);
        txn.commit();
        return postgres_dao_account_modules::IsAffectedRowsPositive(result.affected_rows());
    }
    catch (const std::exception& e)
    {
        std::cerr << "IssueRefreshToken PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

RefreshTokenRotationStatus PostgresDao::RotateRefreshToken(const std::string& selector,
                                                           const std::string& verifier,
                                                           const std::string& replacement_selector,
                                                           const std::string& replacement_verifier_hash,
                                                           int ttl_seconds,
                                                           const std::string& user_agent,
                                                           const std::string& ip_hash,
                                                           int& uid)
{
    uid = 0;
    if (selector.empty() || verifier.empty() || replacement_selector.empty() ||
        !memochat::auth::LooksLikeRefreshTokenHash(replacement_verifier_hash) || ttl_seconds <= 0)
    {
        return RefreshTokenRotationStatus::Invalid;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return RefreshTokenRotationStatus::StorageError;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        pqxx::work txn(*con->_con);
        const auto rows = txn.exec_params("SELECT uid, verifier_hash, expires_at <= now() AS expired, "
                                          "revoked_at IS NOT NULL AS revoked, rotated_at IS NOT NULL AS rotated "
                                          "FROM " +
                                              PgTable("auth_refresh_token") + " WHERE selector = $1 FOR UPDATE",
                                          selector);
        if (rows.empty())
        {
            txn.commit();
            return RefreshTokenRotationStatus::Invalid;
        }

        const auto& row = rows[0];
        uid = row["uid"].as<int>();
        const bool expired = row["expired"].as<bool>();
        const bool revoked = row["revoked"].as<bool>();
        const bool rotated = row["rotated"].as<bool>();
        if (revoked || rotated)
        {
            txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                                 " SET revoked_at = COALESCE(revoked_at, now()) "
                                 "WHERE uid = $1 AND revoked_at IS NULL",
                             uid);
            txn.commit();
            return RefreshTokenRotationStatus::Reused;
        }
        if (expired)
        {
            txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                                 " SET revoked_at = COALESCE(revoked_at, now()) WHERE selector = $1",
                             selector);
            txn.commit();
            return RefreshTokenRotationStatus::Expired;
        }

        const std::string stored_hash = row["verifier_hash"].is_null() ? "" : row["verifier_hash"].c_str();
        if (!memochat::auth::VerifyRefreshTokenVerifier(verifier, stored_hash))
        {
            txn.commit();
            return RefreshTokenRotationStatus::Invalid;
        }

        txn.exec_params0("INSERT INTO " + PgTable("auth_refresh_token") +
                             "(uid, selector, verifier_hash, expires_at, user_agent, ip_hash) "
                             "VALUES ($1, $2, $3, now() + ($4::integer * interval '1 second'), $5, $6)",
                         uid,
                         replacement_selector,
                         replacement_verifier_hash,
                         ttl_seconds,
                         ClampText(user_agent, 512),
                         ip_hash);
        txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                             " SET rotated_at = now(), replaced_by_selector = $1 WHERE selector = $2",
                         replacement_selector,
                         selector);
        txn.commit();
        return RefreshTokenRotationStatus::Success;
    }
    catch (const std::exception& e)
    {
        std::cerr << "RotateRefreshToken PostgreSQL exception: " << e.what() << std::endl;
        uid = 0;
        return RefreshTokenRotationStatus::StorageError;
    }
}

bool PostgresDao::RevokeRefreshToken(const std::string& selector, const std::string& verifier, int& uid)
{
    uid = 0;
    if (selector.empty() || verifier.empty())
    {
        return false;
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
        pqxx::work txn(*con->_con);
        const auto rows = txn.exec_params("SELECT uid, verifier_hash FROM " + PgTable("auth_refresh_token") +
                                              " WHERE selector = $1 FOR UPDATE",
                                          selector);
        if (rows.empty())
        {
            txn.commit();
            return false;
        }
        const auto& row = rows[0];
        const std::string stored_hash = row["verifier_hash"].is_null() ? "" : row["verifier_hash"].c_str();
        if (!memochat::auth::VerifyRefreshTokenVerifier(verifier, stored_hash))
        {
            txn.commit();
            return false;
        }
        uid = row["uid"].as<int>();
        txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                             " SET revoked_at = COALESCE(revoked_at, now()) WHERE selector = $1",
                         selector);
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "RevokeRefreshToken PostgreSQL exception: " << e.what() << std::endl;
        uid = 0;
        return false;
    }
}

bool PostgresDao::RevokeAllRefreshTokensForUid(int uid)
{
    if (uid <= 0)
    {
        return false;
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
        pqxx::work txn(*con->_con);
        txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                             " SET revoked_at = COALESCE(revoked_at, now()) WHERE uid = $1 AND revoked_at IS NULL",
                         uid);
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "RevokeAllRefreshTokensForUid PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        std::cout << "CheckPwd PostgreSQL connection unavailable" << std::endl;
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        pqxx::read_transaction txn(*con->_con);
        const auto rows = txn.exec_params("SELECT uid, name, email, password_hash, user_id, nick, icon, \"desc\", sex "
                                          "FROM " +
                                              PgTable("user") + " WHERE email = $1 LIMIT 1",
                                          email);
        if (rows.empty())
        {
            std::cout << "CheckPwd user not found for email " << email << std::endl;
            return false;
        }

        const auto& row = rows[0];
        const std::string password_hash = row["password_hash"].c_str() ? row["password_hash"].c_str() : "";
        if (!memochat::auth::VerifyPassword(password_hash, pwd))
        {
            return false;
        }

        userInfo.name = row["name"].c_str() ? row["name"].c_str() : "";
        userInfo.email = row["email"].c_str() ? row["email"].c_str() : "";
        userInfo.uid = row["uid"].as<int>();
        userInfo.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
        userInfo.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
        userInfo.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
        userInfo.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
        userInfo.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
        userInfo.pwd.clear();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

std::string PostgresDao::GetUserPublicId(int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return "";
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        pqxx::read_transaction txn(*con->_con);
        const auto rows = txn.exec_params("SELECT user_id FROM \"user\" WHERE uid = $1 LIMIT 1", uid);
        if (rows.empty() || rows[0]["user_id"].is_null())
        {
            return "";
        }
        return rows[0]["user_id"].c_str();
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        return "";
    }
}

void PostgresDao::WarmupAuthQueries()
{
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
        pqxx::read_transaction txn(*con->_con);
        txn.exec_params("SELECT uid, name, email, password_hash, user_id, nick, icon, \"desc\", sex "
                        "FROM \"user\" WHERE email = $1 LIMIT 1",
                        "__memochat_warmup__@invalid.local");
        txn.commit();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DIAG WarmupAuthQueries] PostgreSQL exception: " << e.what() << "\n" << std::flush;
    }
}

std::string PostgresDao::GenerateUserPublicId()
{
    return SnowflakeUtil::formatPublicId(SnowflakeUtil::getInstance().nextId(), 'u');
}

bool PostgresDao::GetUserInfo(int uid, UserInfo& user_info)
{
    if (!postgres_dao_account_modules::HasPositiveUid(uid))
    {
        return false;
    }

    // Account-owning services keep the user table in their pooled DB; services
    // that bridge to memo_account (e.g. moments after the Phase 2 split) read it
    // over a dedicated connection because "user" doesn't exist in their own DB.
    if (postgres_dao_account_modules::ShouldUseAccountBridge(account_connection_string_.empty()))
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT uid, name, email, user_id, nick, icon, \"desc\", sex "
                                              "FROM \"user\" WHERE uid = $1 LIMIT 1",
                                              uid);
            if (rows.empty())
            {
                return false;
            }
            const auto& row = rows[0];
            user_info.uid = row["uid"].as<int>();
            user_info.name = row["name"].is_null() ? "" : row["name"].c_str();
            user_info.email = row["email"].is_null() ? "" : row["email"].c_str();
            user_info.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
            user_info.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
            user_info.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
            user_info.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
            user_info.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "GetUserInfo (account bridge) PostgreSQL exception: " << e.what() << std::endl;
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
        pqxx::read_transaction txn(*con->_con);
        const auto rows = txn.exec_params("SELECT uid, name, email, user_id, nick, icon, \"desc\", sex "
                                          "FROM \"user\" WHERE uid = $1 LIMIT 1",
                                          uid);
        if (rows.empty())
        {
            return false;
        }
        const auto& row = rows[0];
        user_info.uid = row["uid"].as<int>();
        user_info.name = row["name"].is_null() ? "" : row["name"].c_str();
        user_info.email = row["email"].is_null() ? "" : row["email"].c_str();
        user_info.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
        user_info.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
        user_info.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
        user_info.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
        user_info.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "GetUserInfo PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDao::TestProcedure(const std::string& email, int& uid, std::string& name)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        std::cout << "TestProcedure PostgreSQL connection unavailable" << std::endl;
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        pqxx::read_transaction txn(*con->_con);
        const auto rows =
            txn.exec_params("SELECT uid, name FROM " + PgTable("user") + " WHERE email = $1 LIMIT 1", email);
        if (rows.empty())
        {
            std::cout << "TestProcedure user not found for email " << email << std::endl;
            return false;
        }
        uid = rows[0]["uid"].as<int>();
        name = rows[0]["name"].c_str() ? rows[0]["name"].c_str() : "";
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "TestProcedure PostgreSQL exception: " << e.what() << std::endl;
        return false;
    }
}
