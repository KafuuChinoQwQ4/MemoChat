#include "PostgresDao.h"
#include "ConfigMgr.h"
#include "SnowflakeUtil.h"
#include "db/PqxxCompat.h"
#include <string_view>

namespace
{
std::string PostgresSchemaName()
{
    auto& cfg = ConfigMgr::Inst();
    const auto schema = cfg["Postgres"]["Schema"];
    return schema.empty() ? "public" : schema;
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

std::string DecodeLegacyXorPwd(const std::string& input)
{
    unsigned int xor_code = static_cast<unsigned int>(input.size() % 255);
    std::string decoded = input;
    for (size_t i = 0; i < decoded.size(); ++i)
    {
        decoded[i] = static_cast<char>(static_cast<unsigned char>(decoded[i]) ^ xor_code);
    }
    return decoded;
}
} // namespace

int PostgresDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return RegUserTransaction(name, email, pwd, ":/res/head_1.jpg");
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
        for (int i = 0; i < 20; ++i)
        {
            user_public_id = GenerateUserPublicId();
            const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE user_id = $1 LIMIT 1", user_public_id);
            if (rows.empty())
            {
                break;
            }
            user_public_id.clear();
        }

        if (user_public_id.empty())
        {
            std::cout << "generate user_public_id failed" << std::endl;
            return -1;
        }

        const std::string pwd_stored = DecodeLegacyXorPwd(pwd);
        txn.exec_params0("INSERT INTO \"user\"(uid, name, email, pwd, nick, icon, user_id) "
                         "VALUES ($1, $2, $3, $4, $5, $6, $7)",
                         new_id,
                         name,
                         email,
                         pwd_stored,
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
        pqxx::work txn(*con->_con);
        const auto result = txn.exec_params("UPDATE \"user\" SET pwd = $1 WHERE email = $2", newpwd, email);
        txn.commit();
        std::cout << "Updated rows: " << result.affected_rows() << std::endl;
        return result.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
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
        const auto rows = txn.exec_params("SELECT uid, name, email, pwd, user_id, nick, icon, \"desc\", sex "
                                          "FROM " +
                                              PgTable("user") + " WHERE email = $1 LIMIT 1",
                                          email);
        if (rows.empty())
        {
            std::cout << "CheckPwd user not found for email " << email << std::endl;
            return false;
        }

        const auto& row = rows[0];
        const std::string origin_pwd = row["pwd"].c_str() ? row["pwd"].c_str() : "";
        const std::string decoded_pwd = DecodeLegacyXorPwd(origin_pwd);
        if (pwd != origin_pwd && pwd != decoded_pwd)
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
        userInfo.pwd = decoded_pwd;
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
        txn.exec_params("SELECT uid, name, email, pwd, user_id, nick, icon, \"desc\", sex "
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
    if (uid <= 0)
    {
        return false;
    }

    // Account-owning services keep the user table in their pooled DB; services
    // that bridge to memo_account (e.g. moments after the Phase 2 split) read it
    // over a dedicated connection because "user" doesn't exist in their own DB.
    if (!account_connection_string_.empty())
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

bool PostgresDao::TestProcedure(const std::string& email, int& uid, string& name)
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
