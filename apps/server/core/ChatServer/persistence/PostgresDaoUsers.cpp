#include "PostgresDao.h"
#include "db/PqxxCompat.h"
#include "PostgresPool.h"
#include "SnowflakeUtil.h"
#include <pqxx/pqxx>
#include <set>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <stdexcept>
#include <sstream>

namespace
{
bool IsValidUserPublicId(const std::string& user_id)
{
    if (user_id.size() != 10 || user_id[0] != 'u')
    {
        return false;
    }
    if (user_id[1] < '1' || user_id[1] > '9')
    {
        return false;
    }
    for (size_t i = 2; i < user_id.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(user_id[i])))
        {
            return false;
        }
    }
    return true;
}

std::string GenerateUserPublicId()
{
    return SnowflakeUtil::formatPublicId(SnowflakeUtil::getInstance().nextId(), 'u');
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
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::work txn(conn);
            const auto exists = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
            if (!exists.empty())
            {
                txn.commit();
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
                txn.abort();
                return -1;
            }

            txn.exec_params0("INSERT INTO \"user\"(uid, name, email, pwd, nick, icon, user_id) "
                             "VALUES ($1, $2, $3, $4, $5, $6, $7)",
                             new_id,
                             name,
                             email,
                             pwd,
                             name,
                             "",
                             user_public_id);
            txn.commit();
            return new_id;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return -1;
        }
    }
    auto con = pool_->getConnection();
    try
    {
        if (con == nullptr)
        {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));

        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        stmt->execute();

        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next())
        {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool PostgresDao::CheckEmail(const std::string& name, const std::string& email)
{
    (void) name;
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
            return !rows.empty();
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return false;
        }
    }
    auto con = pool_->getConnection();
    try
    {
        if (con == nullptr)
        {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            std::cout << "Check Email: " << res->getString("email") << std::endl;
            if (email != res->getString("email"))
            {
                pool_->returnConnection(std::move(con));
                return false;
            }
            pool_->returnConnection(std::move(con));
            return true;
        }
        return true;
    }
    catch (sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::UpdatePwd(const std::string& email, const std::string& newpwd)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::work txn(conn);
            const auto updated = txn.exec_params("UPDATE \"user\" SET pwd = $1 WHERE email = $2", newpwd, email);
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
    try
    {
        if (con == nullptr)
        {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE email = ?"));

        pstmt->setString(2, email);
        pstmt->setString(1, newpwd);

        int updateCount = pstmt->executeUpdate();

        std::cout << "Updated rows: " << updateCount << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch (sql::SQLException& e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT uid, name, email, pwd, user_id, nick, icon, \"desc\", sex "
                                              "FROM \"user\" WHERE name = $1 LIMIT 1",
                                              name);
            if (rows.empty())
            {
                return false;
            }
            const auto& row = rows[0];
            const std::string origin_pwd = row["pwd"].is_null() ? "" : row["pwd"].c_str();
            if (pwd != origin_pwd)
            {
                const auto decoded_pwd = DecodeLegacyXorPwd(pwd);
                if (decoded_pwd != origin_pwd)
                {
                    return false;
                }
            }

            userInfo.name = row["name"].is_null() ? "" : row["name"].c_str();
            userInfo.email = row["email"].is_null() ? "" : row["email"].c_str();
            userInfo.uid = row["uid"].as<int>();
            userInfo.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
            userInfo.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
            userInfo.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
            userInfo.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
            userInfo.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
            userInfo.pwd = origin_pwd;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return false;
        }

        const std::string origin_pwd = res->getString("pwd");
        if (pwd != origin_pwd)
        {
            return false;
        }

        userInfo.name = name;
        userInfo.email = res->getString("email");
        userInfo.uid = res->getInt("uid");
        userInfo.user_id = res->isNull("user_id") ? "" : res->getString("user_id");
        userInfo.pwd = origin_pwd;
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

bool PostgresDao::AddFriendApply(const int& from, const int& to)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            txn.exec_params("INSERT INTO friend_apply (from_uid, to_uid) VALUES ($1, $2) "
                            "ON CONFLICT (from_uid, to_uid) DO UPDATE SET from_uid = EXCLUDED.from_uid",
                            from,
                            to);
            txn.commit();
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
                                        "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
        pstmt->setInt(1, from); // from id
        pstmt->setInt(2, to);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0)
        {
            return false;
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

    return true;
}

bool PostgresDao::AuthFriendApply(const int& from, const int& to)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE friend_apply SET status = 1 WHERE from_uid = $1 AND to_uid = $2", to, from);
            txn.commit();
            return updated.affected_rows() >= 0;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
                                                                                  "WHERE from_uid = ? AND to_uid = ?"));

        pstmt->setInt(1, to); // from id
        pstmt->setInt(2, from);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0)
        {
            return false;
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

    return true;
}

bool PostgresDao::AddFriend(const int& from, const int& to, std::string back_name)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            txn.exec_params("INSERT INTO friend(self_id, friend_id, back) VALUES ($1, $2, $3) "
                            "ON CONFLICT (self_id, friend_id) DO NOTHING",
                            from,
                            to,
                            back_name);
            txn.exec_params("INSERT INTO friend(self_id, friend_id, back) VALUES ($1, $2, $3) "
                            "ON CONFLICT (self_id, friend_id) DO NOTHING",
                            to,
                            from,
                            "");
            txn.commit();
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
        con->_con->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
                                        "VALUES (?, ?, ?) "));

        pstmt->setInt(1, from); // from id
        pstmt->setInt(2, to);
        pstmt->setString(3, back_name);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0)
        {
            con->_con->rollback();
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt2(
            con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
                                        "VALUES (?, ?, ?) "));

        pstmt2->setInt(1, to); // from id
        pstmt2->setInt(2, from);
        pstmt2->setString(3, "");

        int rowAffected2 = pstmt2->executeUpdate();
        if (rowAffected2 < 0)
        {
            con->_con->rollback();
            return false;
        }

        con->_con->commit();
        std::cout << "addfriend insert friends success" << std::endl;

        return true;
    }
    catch (sql::SQLException& e)
    {
        if (con)
        {
            con->_con->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool PostgresDao::DeleteFriend(const int& from, const int& to)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            txn.exec_params(
                "DELETE FROM friend WHERE (self_id = $1 AND friend_id = $2) OR (self_id = $2 AND friend_id = $1)",
                from,
                to);
            txn.commit();
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
            "DELETE FROM friend WHERE (self_id = ? AND friend_id = ?) OR (self_id = ? AND friend_id = ?)"));
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        pstmt->setInt(3, to);
        pstmt->setInt(4, from);
        pstmt->executeUpdate();
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

bool PostgresDao::ReplaceApplyTags(const int& from, const int& to, const std::vector<std::string>& tags)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            txn.exec_params("DELETE FROM friend_apply_tag WHERE from_uid = $1 AND to_uid = $2", from, to);

            std::set<std::string> uniqTags;
            for (const auto& raw : tags)
            {
                auto begin = raw.find_first_not_of(" \t\r\n");
                if (begin == std::string::npos)
                {
                    continue;
                }
                auto end = raw.find_last_not_of(" \t\r\n");
                std::string tag = raw.substr(begin, end - begin + 1);
                if (tag.size() > 64)
                {
                    tag = tag.substr(0, 64);
                }
                if (!tag.empty())
                {
                    uniqTags.insert(tag);
                }
            }

            for (const auto& tag : uniqTags)
            {
                txn.exec_params("INSERT INTO friend_apply_tag(from_uid, to_uid, tag) VALUES($1, $2, $3) "
                                "ON CONFLICT (from_uid, to_uid, tag) DO NOTHING",
                                from,
                                to,
                                tag);
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
        con->_con->setAutoCommit(false);
        std::unique_ptr<sql::PreparedStatement> delStmt(
            con->_con->prepareStatement("DELETE FROM friend_apply_tag WHERE from_uid = ? AND to_uid = ?"));
        delStmt->setInt(1, from);
        delStmt->setInt(2, to);
        delStmt->executeUpdate();

        std::set<std::string> uniqTags;
        for (const auto& raw : tags)
        {
            auto begin = raw.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
            {
                continue;
            }
            auto end = raw.find_last_not_of(" \t\r\n");
            std::string tag = raw.substr(begin, end - begin + 1);
            if (tag.size() > 64)
            {
                tag = tag.substr(0, 64);
            }
            if (!tag.empty())
            {
                uniqTags.insert(tag);
            }
        }

        if (!uniqTags.empty())
        {
            std::unique_ptr<sql::PreparedStatement> insStmt(con->_con->prepareStatement(
                "INSERT IGNORE INTO friend_apply_tag(from_uid, to_uid, tag) VALUES(?,?,?)"));
            for (const auto& tag : uniqTags)
            {
                insStmt->setInt(1, from);
                insStmt->setInt(2, to);
                insStmt->setString(3, tag);
                insStmt->executeUpdate();
            }
        }

        con->_con->commit();
        return true;
    }
    catch (sql::SQLException& e)
    {
        if (con)
        {
            con->_con->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::ReplaceFriendTags(const int& self_id, const int& friend_id, const std::vector<std::string>& tags)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            txn.exec_params("DELETE FROM friend_tag WHERE self_id = $1 AND friend_id = $2", self_id, friend_id);

            std::set<std::string> uniqTags;
            for (const auto& raw : tags)
            {
                auto begin = raw.find_first_not_of(" \t\r\n");
                if (begin == std::string::npos)
                {
                    continue;
                }
                auto end = raw.find_last_not_of(" \t\r\n");
                std::string tag = raw.substr(begin, end - begin + 1);
                if (tag.size() > 64)
                {
                    tag = tag.substr(0, 64);
                }
                if (!tag.empty())
                {
                    uniqTags.insert(tag);
                }
            }

            for (const auto& tag : uniqTags)
            {
                txn.exec_params("INSERT INTO friend_tag(self_id, friend_id, tag) VALUES($1, $2, $3) "
                                "ON CONFLICT (self_id, friend_id, tag) DO NOTHING",
                                self_id,
                                friend_id,
                                tag);
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
        con->_con->setAutoCommit(false);
        std::unique_ptr<sql::PreparedStatement> delStmt(
            con->_con->prepareStatement("DELETE FROM friend_tag WHERE self_id = ? AND friend_id = ?"));
        delStmt->setInt(1, self_id);
        delStmt->setInt(2, friend_id);
        delStmt->executeUpdate();

        std::set<std::string> uniqTags;
        for (const auto& raw : tags)
        {
            auto begin = raw.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
            {
                continue;
            }
            auto end = raw.find_last_not_of(" \t\r\n");
            std::string tag = raw.substr(begin, end - begin + 1);
            if (tag.size() > 64)
            {
                tag = tag.substr(0, 64);
            }
            if (!tag.empty())
            {
                uniqTags.insert(tag);
            }
        }

        if (!uniqTags.empty())
        {
            std::unique_ptr<sql::PreparedStatement> insStmt(
                con->_con->prepareStatement("INSERT IGNORE INTO friend_tag(self_id, friend_id, tag) VALUES(?,?,?)"));
            for (const auto& tag : uniqTags)
            {
                insStmt->setInt(1, self_id);
                insStmt->setInt(2, friend_id);
                insStmt->setString(3, tag);
                insStmt->executeUpdate();
            }
        }

        con->_con->commit();
        return true;
    }
    catch (sql::SQLException& e)
    {
        if (con)
        {
            con->_con->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::vector<std::string> PostgresDao::GetApplyTags(const int& from, const int& to)
{
    std::vector<std::string> tags;
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows =
                txn.exec_params("SELECT tag FROM friend_apply_tag WHERE from_uid = $1 AND to_uid = $2 ORDER BY id ASC",
                                from,
                                to);
            for (const auto& row : rows)
            {
                tags.push_back(row["tag"].is_null() ? "" : row["tag"].c_str());
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        }
        return tags;
    }
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return tags;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT tag FROM friend_apply_tag WHERE from_uid = ? AND to_uid = ? ORDER BY id ASC"));
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            tags.push_back(res->getString("tag"));
        }
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
    return tags;
}

std::vector<std::string> PostgresDao::GetFriendTags(const int& self_id, const int& friend_id)
{
    std::vector<std::string> tags;
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows =
                txn.exec_params("SELECT tag FROM friend_tag WHERE self_id = $1 AND friend_id = $2 ORDER BY id ASC",
                                self_id,
                                friend_id);
            for (const auto& row : rows)
            {
                tags.push_back(row["tag"].is_null() ? "" : row["tag"].c_str());
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
        }
        return tags;
    }
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return tags;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT tag FROM friend_tag WHERE self_id = ? AND friend_id = ? ORDER BY id ASC"));
        pstmt->setInt(1, self_id);
        pstmt->setInt(2, friend_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            tags.push_back(res->getString("tag"));
        }
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
    return tags;
}
std::shared_ptr<UserInfo> PostgresDao::GetUser(int uid)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT pwd, email, name, nick, \"desc\", sex, icon, user_id, uid "
                                              "FROM \"user\" WHERE uid = $1 LIMIT 1",
                                              uid);
            if (rows.empty())
            {
                return nullptr;
            }

            const auto& row = rows[0];
            auto user_ptr = std::make_shared<UserInfo>();
            user_ptr->pwd = row["pwd"].is_null() ? "" : row["pwd"].c_str();
            user_ptr->email = row["email"].is_null() ? "" : row["email"].c_str();
            user_ptr->name = row["name"].is_null() ? "" : row["name"].c_str();
            user_ptr->nick = row["nick"].is_null() ? "" : row["nick"].c_str();
            user_ptr->desc = row["desc"].is_null() ? "" : row["desc"].c_str();
            user_ptr->sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
            user_ptr->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
            user_ptr->user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
            user_ptr->uid = row["uid"].as<int>();
            return user_ptr;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return nullptr;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;

        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            user_ptr->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
            user_ptr->uid = uid;
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> PostgresDao::GetUser(std::string name)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT pwd, email, name, nick, \"desc\", sex, icon, user_id, uid "
                                              "FROM \"user\" WHERE name = $1 LIMIT 1",
                                              name);
            if (rows.empty())
            {
                return nullptr;
            }

            const auto& row = rows[0];
            auto user_ptr = std::make_shared<UserInfo>();
            user_ptr->pwd = row["pwd"].is_null() ? "" : row["pwd"].c_str();
            user_ptr->email = row["email"].is_null() ? "" : row["email"].c_str();
            user_ptr->name = row["name"].is_null() ? "" : row["name"].c_str();
            user_ptr->nick = row["nick"].is_null() ? "" : row["nick"].c_str();
            user_ptr->desc = row["desc"].is_null() ? "" : row["desc"].c_str();
            user_ptr->sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
            user_ptr->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
            user_ptr->user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
            user_ptr->uid = row["uid"].as<int>();
            return user_ptr;
        }
        catch (const std::exception& e)
        {
            std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
            return nullptr;
        }
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;

        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            user_ptr->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
            user_ptr->uid = res->getInt("uid");
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

bool PostgresDao::GetUidByUserId(const std::string& user_id, int& uid)
{
    uid = 0;
    if (!IsValidUserPublicId(user_id))
    {
        return false;
    }

    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(account_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT uid FROM \"user\" WHERE user_id = $1 LIMIT 1", user_id);
            if (rows.empty())
            {
                return false;
            }
            uid = rows[0]["uid"].as<int>();
            return uid > 0;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT uid FROM user WHERE user_id = ? LIMIT 1"));
        pstmt->setString(1, user_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return false;
        }
        uid = res->getInt("uid");
        return uid > 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            std::vector<int> from_uids;
            std::unordered_map<int, std::shared_ptr<ApplyInfo>> apply_by_uid;
            // Step 1: apply rows (from_uid + status), no JOIN to "user".
            struct ApplyRow
            {
                int uid;
                int status;
            };
            std::vector<ApplyRow> apply_rows;
            const auto rows = txn.exec_params("SELECT apply.from_uid, apply.status "
                                              "FROM friend_apply AS apply "
                                              "WHERE apply.to_uid = $1 AND apply.id > $2 "
                                              "ORDER BY apply.id ASC LIMIT $3",
                                              touid,
                                              begin,
                                              limit);
            for (const auto& row : rows)
            {
                apply_rows.push_back(
                    {row["from_uid"].as<int>(), row["status"].is_null() ? 0 : row["status"].as<int>()});
                from_uids.push_back(row["from_uid"].as<int>());
            }
            // Step 2: batch user base-info (account-data seam, replaces JOIN).
            auto users = GetUsersByUids(from_uids);
            for (const auto& ar : apply_rows)
            {
                const auto uit = users.find(ar.uid);
                const std::string name = (uit != users.end() && uit->second) ? uit->second->name : "";
                const std::string icon = (uit != users.end() && uit->second) ? uit->second->icon : "";
                const std::string nick = (uit != users.end() && uit->second) ? uit->second->nick : "";
                const int sex = (uit != users.end() && uit->second) ? uit->second->sex : 0;
                const std::string user_public_id = (uit != users.end() && uit->second) ? uit->second->user_id : "";
                auto apply_ptr =
                    std::make_shared<ApplyInfo>(ar.uid, name, "", icon, nick, sex, ar.status, user_public_id);
                applyList.push_back(apply_ptr);
                apply_by_uid.emplace(ar.uid, apply_ptr);
            }

            if (!from_uids.empty())
            {
                pqxx::params tag_params;
                tag_params.append(touid);
                std::string in_clause;
                for (size_t i = 0; i < from_uids.size(); ++i)
                {
                    if (i > 0)
                    {
                        in_clause += ", ";
                    }
                    in_clause += "$" + std::to_string(i + 2);
                    tag_params.append(from_uids[i]);
                }
                const auto tag_rows =
                    txn.exec("SELECT from_uid, tag FROM friend_apply_tag WHERE to_uid = $1 AND from_uid IN (" +
                                 in_clause + ") ORDER BY id ASC",
                             tag_params);
                for (const auto& row : tag_rows)
                {
                    const auto uid = row["from_uid"].as<int>();
                    const auto it = apply_by_uid.find(uid);
                    if (it != apply_by_uid.end() && it->second)
                    {
                        it->second->_labels.push_back(row["tag"].is_null() ? "" : row["tag"].c_str());
                    }
                }
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
        std::vector<int> from_uids;
        std::unordered_map<int, std::shared_ptr<ApplyInfo>> apply_by_uid;

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
                                        "user.nick, user.sex, user.user_id, user.icon "
                                        "from friend_apply as apply join user on "
                                        "apply.from_uid = user.uid where apply.to_uid = ? "
                                        "and apply.id > ? order by apply.id ASC LIMIT ? "));

        pstmt->setInt(1, touid);
        pstmt->setInt(2, begin);
        pstmt->setInt(3, limit);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            auto name = res->getString("name");
            auto uid = res->getInt("from_uid");
            auto status = res->getInt("status");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto user_id = res->isNull("user_id") ? "" : res->getString("user_id");
            auto icon = res->isNull("icon") ? "" : res->getString("icon");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", icon, nick, sex, status, user_id);
            applyList.push_back(apply_ptr);
            from_uids.push_back(uid);
            apply_by_uid.emplace(uid, apply_ptr);
        }

        if (!from_uids.empty())
        {
            std::string placeholders;
            placeholders.reserve(from_uids.size() * 2);
            for (size_t i = 0; i < from_uids.size(); ++i)
            {
                if (i > 0)
                {
                    placeholders += ",";
                }
                placeholders += "?";
            }

            std::unique_ptr<sql::PreparedStatement> tag_stmt(con->_con->prepareStatement(
                "SELECT from_uid, tag FROM friend_apply_tag WHERE to_uid = ? AND from_uid IN (" + placeholders +
                ") ORDER BY id ASC"));
            tag_stmt->setInt(1, touid);
            for (size_t i = 0; i < from_uids.size(); ++i)
            {
                tag_stmt->setInt(static_cast<int>(i + 2), from_uids[i]);
            }
            std::unique_ptr<sql::ResultSet> tag_res(tag_stmt->executeQuery());
            while (tag_res->next())
            {
                const auto uid = tag_res->getInt("from_uid");
                const auto it = apply_by_uid.find(uid);
                if (it != apply_by_uid.end() && it->second)
                {
                    it->second->_labels.push_back(tag_res->getString("tag"));
                }
            }
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

std::unordered_map<int, std::shared_ptr<UserInfo>> PostgresDao::GetUsersByUids(const std::vector<int>& uids)
{
    std::unordered_map<int, std::shared_ptr<UserInfo>> result;
    if (uids.empty() || !use_postgres_)
    {
        return result;
    }
    try
    {
        pqxx::connection conn(account_connection_string_);
        pqxx::read_transaction txn(conn);
        pqxx::params params;
        std::string in_clause;
        for (size_t i = 0; i < uids.size(); ++i)
        {
            if (i > 0)
            {
                in_clause += ", ";
            }
            in_clause += "$" + std::to_string(i + 1);
            params.append(uids[i]);
        }
        // Single user-table read replacing per-query JOIN "user". When the user
        // table moves to memo_account (Phase 2b), only this query relocates.
        const auto rows = txn.exec("SELECT uid, name, nick, sex, user_id, \"desc\", icon "
                                   "FROM \"user\" WHERE uid IN (" +
                                       in_clause + ")",
                                   params);
        for (const auto& row : rows)
        {
            auto info = std::make_shared<UserInfo>();
            info->uid = row["uid"].as<int>();
            info->name = row["name"].is_null() ? "" : row["name"].c_str();
            info->nick = row["nick"].is_null() ? "" : row["nick"].c_str();
            info->sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
            info->user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
            info->desc = row["desc"].is_null() ? "" : row["desc"].c_str();
            info->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
            result.emplace(info->uid, info);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "GetUsersByUids failed: " << e.what() << std::endl;
    }
    return result;
}

bool PostgresDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            std::vector<int> friend_ids;
            std::unordered_map<int, std::shared_ptr<UserInfo>> friend_by_id;
            std::unordered_map<int, std::string> back_by_id;
            // Step 1: relation rows (friend ids + back name) — no JOIN to "user".
            const auto rows =
                txn.exec_params("SELECT f.friend_id, f.back FROM friend AS f WHERE f.self_id = $1", self_id);
            for (const auto& row : rows)
            {
                const auto friend_id = row["friend_id"].as<int>();
                friend_ids.push_back(friend_id);
                back_by_id.emplace(friend_id, row["back"].is_null() ? "" : row["back"].c_str());
            }
            // Step 2: resolve user base-info in one batch read (account-data seam).
            auto users = GetUsersByUids(friend_ids);
            for (const auto friend_id : friend_ids)
            {
                auto user_info = std::make_shared<UserInfo>();
                user_info->uid = friend_id;
                const auto uit = users.find(friend_id);
                if (uit != users.end() && uit->second)
                {
                    user_info->name = uit->second->name;
                    user_info->nick = uit->second->nick;
                    user_info->sex = uit->second->sex;
                    user_info->user_id = uit->second->user_id;
                    user_info->desc = uit->second->desc;
                    user_info->icon = uit->second->icon;
                }
                user_info->back = back_by_id[friend_id];
                user_info_list.push_back(user_info);
                friend_by_id.emplace(friend_id, user_info);
            }

            if (!friend_ids.empty())
            {
                pqxx::params tag_params;
                tag_params.append(self_id);
                std::string in_clause;
                for (size_t i = 0; i < friend_ids.size(); ++i)
                {
                    if (i > 0)
                    {
                        in_clause += ", ";
                    }
                    in_clause += "$" + std::to_string(i + 2);
                    tag_params.append(friend_ids[i]);
                }
                const auto tag_rows =
                    txn.exec("SELECT friend_id, tag FROM friend_tag WHERE self_id = $1 AND friend_id IN (" + in_clause +
                                 ") ORDER BY id ASC",
                             tag_params);
                for (const auto& row : tag_rows)
                {
                    const auto friend_id = row["friend_id"].as<int>();
                    const auto it = friend_by_id.find(friend_id);
                    if (it != friend_by_id.end() && it->second)
                    {
                        it->second->labels.push_back(row["tag"].is_null() ? "" : row["tag"].c_str());
                    }
                }
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
        std::vector<int> friend_ids;
        std::unordered_map<int, std::shared_ptr<UserInfo>> friend_by_id;

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT f.friend_id, f.back, u.name, u.nick, u.sex, u.user_id, u.`desc`, u.icon "
            "FROM friend AS f "
            "JOIN user AS u ON f.friend_id = u.uid "
            "WHERE f.self_id = ?"));

        pstmt->setInt(1, self_id);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            auto friend_id = res->getInt("friend_id");
            auto back = res->isNull("back") ? "" : res->getString("back");
            auto user_info = std::make_shared<UserInfo>();
            user_info->uid = friend_id;
            user_info->name = res->getString("name");
            user_info->nick = res->isNull("nick") ? "" : res->getString("nick");
            user_info->sex = res->getInt("sex");
            user_info->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
            user_info->desc = res->isNull("desc") ? "" : res->getString("desc");
            user_info->icon = res->isNull("icon") ? "" : res->getString("icon");
            user_info->back = back;
            user_info_list.push_back(user_info);
            friend_ids.push_back(friend_id);
            friend_by_id.emplace(friend_id, user_info);
        }

        if (!friend_ids.empty())
        {
            std::string placeholders;
            placeholders.reserve(friend_ids.size() * 2);
            for (size_t i = 0; i < friend_ids.size(); ++i)
            {
                if (i > 0)
                {
                    placeholders += ",";
                }
                placeholders += "?";
            }

            std::unique_ptr<sql::PreparedStatement> tag_stmt(con->_con->prepareStatement(
                "SELECT friend_id, tag FROM friend_tag WHERE self_id = ? AND friend_id IN (" + placeholders +
                ") ORDER BY id ASC"));
            tag_stmt->setInt(1, self_id);
            for (size_t i = 0; i < friend_ids.size(); ++i)
            {
                tag_stmt->setInt(static_cast<int>(i + 2), friend_ids[i]);
            }
            std::unique_ptr<sql::ResultSet> tag_res(tag_stmt->executeQuery());
            while (tag_res->next())
            {
                const auto friend_id = tag_res->getInt("friend_id");
                const auto it = friend_by_id.find(friend_id);
                if (it != friend_by_id.end() && it->second)
                {
                    it->second->labels.push_back(tag_res->getString("tag"));
                }
            }
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

    return true;
}

bool PostgresDao::IsFriend(const int& self_id, const int& friend_id)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT 1 FROM friend WHERE self_id = $1 AND friend_id = $2 LIMIT 1",
                                              self_id,
                                              friend_id);
            return !rows.empty();
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT 1 FROM friend WHERE self_id = ? AND friend_id = ? LIMIT 1"));
        pstmt->setInt(1, self_id);
        pstmt->setInt(2, friend_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::vector<int> PostgresDao::FilterFriendUids(int viewer_uid, const std::vector<int>& author_uids)
{
    // Returns the subset of author_uids visible to viewer_uid under the moments
    // "friends-only" rule: a bidirectional `friend` row OR an accepted (status=1)
    // friend_apply in either direction. Mirrors the EXISTS logic the moments feed
    // used to embed inline, so visibility semantics are unchanged after the feed
    // query stops touching the friend tables directly.
    std::vector<int> result;
    if (viewer_uid <= 0 || author_uids.empty() || !use_postgres_)
    {
        return result;
    }
    try
    {
        pqxx::connection conn(postgres_connection_string_);
        pqxx::read_transaction txn(conn);
        pqxx::params params;
        params.append(viewer_uid);
        std::string in_clause;
        for (size_t i = 0; i < author_uids.size(); ++i)
        {
            if (i > 0)
            {
                in_clause += ", ";
            }
            in_clause += "$" + std::to_string(i + 2);
            params.append(author_uids[i]);
        }
        const auto rows = txn.exec(
            "SELECT a.uid FROM (SELECT unnest(ARRAY[" + in_clause + "]::int[]) AS uid) AS a "
            "WHERE EXISTS ("
            "    SELECT 1 FROM friend f "
            "    WHERE ((f.self_id = a.uid AND f.friend_id = $1) OR (f.self_id = $1 AND f.friend_id = a.uid))"
            ") "
            "OR EXISTS ("
            "    SELECT 1 FROM friend_apply fa "
            "    WHERE fa.status = 1 "
            "      AND ((fa.from_uid = a.uid AND fa.to_uid = $1) OR (fa.from_uid = $1 AND fa.to_uid = a.uid))"
            ")",
            params);
        result.reserve(rows.size());
        for (const auto& row : rows)
        {
            result.push_back(row["uid"].as<int>());
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "FilterFriendUids PostgreSQL exception: " << e.what() << std::endl;
        result.clear();
    }
    return result;
}
