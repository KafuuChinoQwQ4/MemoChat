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
template <typename Transaction> bool TransactionOk(const char* operation, const Transaction& txn)
{
    if (txn.ok())
    {
        return true;
    }
    std::cerr << operation << " PostgreSQL error: " << txn.error_message() << std::endl;
    return false;
}

template <typename Transaction> bool CommitTransaction(const char* operation, Transaction& txn)
{
    if (!TransactionOk(operation, txn))
    {
        return false;
    }
    if (txn.commit())
    {
        return true;
    }
    return TransactionOk(operation, txn);
}

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

    std::string password_hash;
    if (!memochat::auth::HashPassword(pwd, password_hash))
    {
        return -1;
    }

    pqxx::work txn(*con->_con);
    const auto exists = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
    if (!TransactionOk("RegUserTransaction", txn))
    {
        return -1;
    }
    if (!exists.empty())
    {
        if (!txn.commit())
        {
            TransactionOk("RegUserTransaction", txn);
            return -1;
        }
        std::cout << "email " << email << " exists" << std::endl;
        return 0;
    }

    const auto id_row = txn.exec1("SELECT COALESCE(MAX(id), 1000) + 1 FROM user_id");
    const int new_id = id_row[0].as<int>();
    txn.exec0("DELETE FROM user_id");
    txn.exec_params0("INSERT INTO user_id(id) VALUES ($1)", new_id);
    if (!TransactionOk("RegUserTransaction", txn))
    {
        return -1;
    }

    std::string user_public_id;
    for (int i = 0; i < postgres_dao_account_modules::UserPublicIdMaxAttempts(); ++i)
    {
        user_public_id = GenerateUserPublicId();
        const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE user_id = $1 LIMIT 1", user_public_id);
        if (!TransactionOk("RegUserTransaction", txn))
        {
            return -1;
        }
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
    if (!TransactionOk("RegUserTransaction", txn) || !txn.commit())
    {
        TransactionOk("RegUserTransaction", txn);
        return -1;
    }
    std::cout << "newuser insert into user success" << std::endl;
    return new_id;
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
    return TransactionOk("CheckEmail", txn) && !rows.empty();
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
    if (!TransactionOk("UpdatePwd", txn))
    {
        return false;
    }
    if (!result.empty())
    {
        const int uid = result[0]["uid"].as<int>();
        txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                             " SET revoked_at = now() WHERE uid = $1 AND revoked_at IS NULL",
                         uid);
    }
    const bool updated =
        TransactionOk("UpdatePwd", txn) && postgres_dao_account_modules::IsAffectedRowsPositive(result.affected_rows());
    if (!txn.commit())
    {
        TransactionOk("UpdatePwd", txn);
        return false;
    }
    std::cout << "Updated rows: " << result.affected_rows() << std::endl;
    return updated;
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

    pqxx::work txn(*con->_con);
    const auto result = txn.exec_params("INSERT INTO " + PgTable("auth_refresh_token") +
                                            "(uid, selector, verifier_hash, expires_at, user_agent, ip_hash) "
                                            "VALUES ($1, $2, $3, now() + ($4::integer * interval '1 second'), $5, $6) "
                                            "ON CONFLICT (selector) DO NOTHING",
                                        uid,
                                        selector,
                                        verifier_hash,
                                        ttl_seconds,
                                        ClampText(user_agent, 512),
                                        ip_hash);
    const bool issued = TransactionOk("IssueRefreshToken", txn) &&
                        postgres_dao_account_modules::IsAffectedRowsPositive(result.affected_rows());
    return txn.commit() && issued;
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

    pqxx::work txn(*con->_con);
    const auto rows = txn.exec_params("SELECT uid, verifier_hash, expires_at <= now() AS expired, "
                                      "revoked_at IS NOT NULL AS revoked, rotated_at IS NOT NULL AS rotated "
                                      "FROM " +
                                          PgTable("auth_refresh_token") + " WHERE selector = $1 FOR UPDATE",
                                      selector);
    if (!TransactionOk("RotateRefreshToken", txn))
    {
        return RefreshTokenRotationStatus::StorageError;
    }
    if (rows.empty())
    {
        if (!CommitTransaction("RotateRefreshToken", txn))
        {
            return RefreshTokenRotationStatus::StorageError;
        }
        return RefreshTokenRotationStatus::Invalid;
    }

    const auto& row = rows[0];
    uid = row["uid"].as<int>();
    const bool expired = row["expired"].as<bool>();
    const bool revoked = row["revoked"].as<bool>();
    const bool rotated = row["rotated"].as<bool>();
    if (!TransactionOk("RotateRefreshToken", txn))
    {
        uid = 0;
        return RefreshTokenRotationStatus::StorageError;
    }
    if (revoked || rotated)
    {
        txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                             " SET revoked_at = COALESCE(revoked_at, now()) "
                             "WHERE uid = $1 AND revoked_at IS NULL",
                         uid);
        if (!CommitTransaction("RotateRefreshToken", txn))
        {
            uid = 0;
            return RefreshTokenRotationStatus::StorageError;
        }
        return RefreshTokenRotationStatus::Reused;
    }
    if (expired)
    {
        txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                             " SET revoked_at = COALESCE(revoked_at, now()) WHERE selector = $1",
                         selector);
        if (!CommitTransaction("RotateRefreshToken", txn))
        {
            uid = 0;
            return RefreshTokenRotationStatus::StorageError;
        }
        return RefreshTokenRotationStatus::Expired;
    }

    const std::string stored_hash = row["verifier_hash"].is_null() ? "" : row["verifier_hash"].c_str();
    if (!memochat::auth::VerifyRefreshTokenVerifier(verifier, stored_hash))
    {
        if (!CommitTransaction("RotateRefreshToken", txn))
        {
            uid = 0;
            return RefreshTokenRotationStatus::StorageError;
        }
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
    if (!CommitTransaction("RotateRefreshToken", txn))
    {
        uid = 0;
        return RefreshTokenRotationStatus::StorageError;
    }
    return RefreshTokenRotationStatus::Success;
}

bool PostgresDao::ResolveActiveRefreshTokenUserId(const std::string& selector, const std::string& verifier, int& uid)
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT uid, verifier_hash FROM " + PgTable("auth_refresh_token") +
                                          " WHERE selector = $1 AND expires_at > now() "
                                          "AND revoked_at IS NULL AND rotated_at IS NULL LIMIT 1",
                                      selector);
    if (!TransactionOk("ResolveActiveRefreshTokenUserId", txn) || rows.empty())
    {
        return false;
    }
    const std::string stored_hash = rows[0]["verifier_hash"].is_null() ? "" : rows[0]["verifier_hash"].c_str();
    if (!memochat::auth::VerifyRefreshTokenVerifier(verifier, stored_hash))
    {
        return false;
    }
    uid = rows[0]["uid"].as<int>();
    return uid > 0;
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

    pqxx::work txn(*con->_con);
    const auto rows = txn.exec_params("SELECT uid, verifier_hash FROM " + PgTable("auth_refresh_token") +
                                          " WHERE selector = $1 FOR UPDATE",
                                      selector);
    if (!TransactionOk("RevokeRefreshToken", txn) || rows.empty())
    {
        txn.abort();
        return false;
    }
    const auto& row = rows[0];
    const std::string stored_hash = row["verifier_hash"].is_null() ? "" : row["verifier_hash"].c_str();
    if (!memochat::auth::VerifyRefreshTokenVerifier(verifier, stored_hash))
    {
        txn.abort();
        return false;
    }
    uid = row["uid"].as<int>();
    txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                         " SET revoked_at = COALESCE(revoked_at, now()) WHERE selector = $1",
                     selector);
    if (!TransactionOk("RevokeRefreshToken", txn) || !txn.commit())
    {
        uid = 0;
        return false;
    }
    return true;
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

    pqxx::work txn(*con->_con);
    txn.exec_params0("UPDATE " + PgTable("auth_refresh_token") +
                         " SET revoked_at = COALESCE(revoked_at, now()) WHERE uid = $1 AND revoked_at IS NULL",
                     uid);
    return TransactionOk("RevokeAllRefreshTokensForUid", txn) && txn.commit();
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT uid, name, email, password_hash, user_id, nick, icon, \"desc\", sex "
                                      "FROM " +
                                          PgTable("user") + " WHERE email = $1 LIMIT 1",
                                      email);
    if (!TransactionOk("CheckPwd", txn) || rows.empty())
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
    return TransactionOk("CheckPwd", txn);
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT user_id FROM \"user\" WHERE uid = $1 LIMIT 1", uid);
    if (!TransactionOk("GetUserPublicId", txn) || rows.empty() || rows[0]["user_id"].is_null())
    {
        return "";
    }
    return rows[0]["user_id"].c_str();
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

    pqxx::read_transaction txn(*con->_con);
    txn.exec_params("SELECT uid, name, email, password_hash, user_id, nick, icon, \"desc\", sex "
                    "FROM \"user\" WHERE email = $1 LIMIT 1",
                    "__memochat_warmup__@invalid.local");
    if (!TransactionOk("WarmupAuthQueries", txn) || !txn.commit())
    {
        TransactionOk("WarmupAuthQueries", txn);
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
        pqxx::connection conn(account_connection_string_);
        if (!conn.is_open())
        {
            std::cerr << "GetUserInfo account bridge PostgreSQL error: " << conn.error_message() << std::endl;
            return false;
        }
        pqxx::read_transaction txn(conn);
        const auto rows = txn.exec_params("SELECT uid, name, email, user_id, nick, icon, \"desc\", sex "
                                          "FROM \"user\" WHERE uid = $1 LIMIT 1",
                                          uid);
        if (!TransactionOk("GetUserInfo account bridge", txn) || rows.empty())
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
        return TransactionOk("GetUserInfo account bridge", txn);
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT uid, name, email, user_id, nick, icon, \"desc\", sex "
                                      "FROM \"user\" WHERE uid = $1 LIMIT 1",
                                      uid);
    if (!TransactionOk("GetUserInfo", txn) || rows.empty())
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
    return TransactionOk("GetUserInfo", txn);
}

bool PostgresDao::GetR18AccessPolicy(int uid, R18AccessPolicyInfo& policy)
{
    if (!postgres_dao_account_modules::HasPositiveUid(uid))
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT adult_attested_at_ms, r18_access_state FROM " + PgTable("user") +
                                          " WHERE uid = $1 LIMIT 1",
                                      uid);
    if (!TransactionOk("GetR18AccessPolicy", txn) || rows.empty())
    {
        return false;
    }

    policy.adult_attested_at_ms =
        rows[0]["adult_attested_at_ms"].is_null() ? 0 : rows[0]["adult_attested_at_ms"].as<int64_t>();
    policy.state = rows[0]["r18_access_state"].is_null() ? 0 : rows[0]["r18_access_state"].as<int>();
    return policy.state >= 0 && policy.state <= 2;
}

bool PostgresDao::AttestAdultForR18(int uid, int64_t attested_at_ms, R18AccessPolicyInfo& policy)
{
    if (!postgres_dao_account_modules::HasPositiveUid(uid) || attested_at_ms <= 0)
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

    pqxx::work txn(*con->_con);
    const auto rows = txn.exec_params(
        "UPDATE " + PgTable("user") +
            " SET adult_attested_at_ms = CASE WHEN adult_attested_at_ms > 0 THEN adult_attested_at_ms ELSE $2 END, "
            "r18_access_state = CASE WHEN r18_access_state = 2 THEN 2 ELSE 1 END "
            "WHERE uid = $1 RETURNING adult_attested_at_ms, r18_access_state",
        uid,
        attested_at_ms);
    if (!TransactionOk("AttestAdultForR18", txn) || rows.empty())
    {
        return false;
    }

    policy.adult_attested_at_ms = rows[0]["adult_attested_at_ms"].as<int64_t>();
    policy.state = rows[0]["r18_access_state"].as<int>();
    if (policy.state < 0 || policy.state > 2 || !CommitTransaction("AttestAdultForR18", txn))
    {
        return false;
    }
    return true;
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

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT uid, name FROM " + PgTable("user") + " WHERE email = $1 LIMIT 1", email);
    if (!TransactionOk("TestProcedure", txn) || rows.empty())
    {
        std::cout << "TestProcedure user not found for email " << email << std::endl;
        return false;
    }
    uid = rows[0]["uid"].as<int>();
    name = rows[0]["name"].c_str() ? rows[0]["name"].c_str() : "";
    return TransactionOk("TestProcedure", txn);
}
