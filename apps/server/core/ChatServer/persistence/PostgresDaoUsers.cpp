#include "PostgresDao.hpp"
#include "auth/PasswordHasher.hpp"
#include "db/PqxxCompat.hpp"
#include "SnowflakeUtil.hpp"
#include <set>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <sstream>

import memochat.chat.postgres_dao_users_algorithms;

namespace postgres_dao_users_modules = memochat::chat::persistence::postgres_dao_users::modules;

namespace
{
bool IsValidUserPublicId(const std::string& user_id)
{
    return postgres_dao_users_modules::IsValidUserPublicIdShape(user_id.c_str(), static_cast<int>(user_id.size()));
}

std::string GenerateUserPublicId()
{
    return SnowflakeUtil::formatPublicId(SnowflakeUtil::getInstance().nextId(), 'u');
}
} // namespace
int PostgresDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    std::string password_hash;
    if (!memochat::auth::HashPassword(pwd, password_hash))
    {
        return -1;
    }

    pqxx::connection conn(account_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }
    const auto exists = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
    if (!exists.ok())
    {
        const auto& postgres_error = exists.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }
    if (!exists.empty())
    {
        if (!txn.commit())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return -1;
        }
        return 0;
    }

    const int new_id = txn.exec1("SELECT COALESCE(MAX(id), 1000) + 1 FROM user_id")[0].as<int>();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }
    txn.exec0("DELETE FROM user_id");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }
    txn.exec_params0("INSERT INTO user_id(id) VALUES ($1)", new_id);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }

    std::string user_public_id;
    for (int i = 0; i < 20; ++i)
    {
        user_public_id = GenerateUserPublicId();
        const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE user_id = $1 LIMIT 1", user_public_id);
        if (!rows.ok())
        {
            const auto& postgres_error = rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return -1;
        }
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

    txn.exec_params0("INSERT INTO \"user\"(uid, name, email, pwd, password_hash, nick, icon, user_id) "
                     "VALUES ($1, $2, $3, '', $4, $5, $6, $7)",
                     new_id,
                     name,
                     email,
                     password_hash,
                     name,
                     "",
                     user_public_id);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return -1;
    }
    return new_id;
}

bool PostgresDao::CheckEmail(const std::string& name, const std::string& email)
{
    (void) name;

    pqxx::connection conn(account_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE email = $1 LIMIT 1", email);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return !rows.empty();
}

bool PostgresDao::UpdatePwd(const std::string& email, const std::string& newpwd)
{
    std::string password_hash;
    if (!memochat::auth::HashPassword(newpwd, password_hash))
    {
        return false;
    }

    pqxx::connection conn(account_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated =
        txn.exec_params("UPDATE \"user\" SET password_hash = $1, pwd = '' WHERE email = $2", password_hash, email);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return updated.affected_rows() > 0;
}

bool PostgresDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo)
{
    pqxx::connection conn(account_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params("SELECT uid, name, email, password_hash, user_id, nick, icon, \"desc\", sex "
                                      "FROM \"user\" WHERE name = $1 LIMIT 1",
                                      name);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return false;
    }
    const auto& row = rows[0];
    const std::string password_hash = row["password_hash"].is_null() ? "" : row["password_hash"].c_str();
    if (!memochat::auth::VerifyPassword(password_hash, pwd))
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return false;
    }

    userInfo.name = row["name"].is_null() ? "" : row["name"].c_str();
    userInfo.email = row["email"].is_null() ? "" : row["email"].c_str();
    userInfo.uid = row["uid"].as<int>();
    userInfo.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
    userInfo.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
    userInfo.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
    userInfo.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
    userInfo.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
    userInfo.pwd.clear();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::AddFriendApply(const int& from, const int& to)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO friend_apply (from_uid, to_uid) VALUES ($1, $2) "
                    "ON CONFLICT (from_uid, to_uid) DO UPDATE SET from_uid = EXCLUDED.from_uid",
                    from,
                    to);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::AuthFriendApply(const int& from, const int& to)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated =
        txn.exec_params("UPDATE friend_apply SET status = 1 WHERE from_uid = $1 AND to_uid = $2", to, from);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::AddFriend(const int& from, const int& to, std::string back_name)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO friend(self_id, friend_id, back) VALUES ($1, $2, $3) "
                    "ON CONFLICT (self_id, friend_id) DO NOTHING",
                    from,
                    to,
                    back_name);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO friend(self_id, friend_id, back) VALUES ($1, $2, $3) "
                    "ON CONFLICT (self_id, friend_id) DO NOTHING",
                    to,
                    from,
                    "");
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::DeleteFriend(const int& from, const int& to)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("DELETE FROM friend WHERE (self_id = $1 AND friend_id = $2) OR (self_id = $2 AND friend_id = $1)",
                    from,
                    to);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::ReplaceApplyTags(const int& from, const int& to, const std::vector<std::string>& tags)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("DELETE FROM friend_apply_tag WHERE from_uid = $1 AND to_uid = $2", from, to);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }

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
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::ReplaceFriendTags(const int& self_id, const int& friend_id, const std::vector<std::string>& tags)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("DELETE FROM friend_tag WHERE self_id = $1 AND friend_id = $2", self_id, friend_id);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }

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
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    if (!txn.commit())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

std::vector<std::string> PostgresDao::GetApplyTags(const int& from, const int& to)
{
    std::vector<std::string> tags;

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return tags;
    }
    const auto rows =
        txn.exec_params("SELECT tag FROM friend_apply_tag WHERE from_uid = $1 AND to_uid = $2 ORDER BY id ASC",
                        from,
                        to);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return tags;
    }
    for (const auto& row : rows)
    {
        tags.push_back(row["tag"].is_null() ? "" : row["tag"].c_str());
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return tags;
    }

    return tags;
}

std::vector<std::string> PostgresDao::GetFriendTags(const int& self_id, const int& friend_id)
{
    std::vector<std::string> tags;

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return tags;
    }
    const auto rows =
        txn.exec_params("SELECT tag FROM friend_tag WHERE self_id = $1 AND friend_id = $2 ORDER BY id ASC",
                        self_id,
                        friend_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return tags;
    }
    for (const auto& row : rows)
    {
        tags.push_back(row["tag"].is_null() ? "" : row["tag"].c_str());
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return tags;
    }

    return tags;
}
std::shared_ptr<UserInfo> PostgresDao::GetUser(int uid)
{
    pqxx::connection conn(account_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return nullptr;
    }
    const auto rows = txn.exec_params("SELECT email, name, nick, \"desc\", sex, icon, user_id, uid "
                                      "FROM \"user\" WHERE uid = $1 LIMIT 1",
                                      uid);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return nullptr;
    }
    if (rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return nullptr;
        }
        return nullptr;
    }

    const auto& row = rows[0];
    auto user_ptr = std::make_shared<UserInfo>();
    user_ptr->email = row["email"].is_null() ? "" : row["email"].c_str();
    user_ptr->name = row["name"].is_null() ? "" : row["name"].c_str();
    user_ptr->nick = row["nick"].is_null() ? "" : row["nick"].c_str();
    user_ptr->desc = row["desc"].is_null() ? "" : row["desc"].c_str();
    user_ptr->sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
    user_ptr->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
    user_ptr->user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
    user_ptr->uid = row["uid"].as<int>();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return nullptr;
    }
    return user_ptr;
}

std::shared_ptr<UserInfo> PostgresDao::GetUser(std::string name)
{
    pqxx::connection conn(account_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return nullptr;
    }
    const auto rows = txn.exec_params("SELECT email, name, nick, \"desc\", sex, icon, user_id, uid "
                                      "FROM \"user\" WHERE name = $1 LIMIT 1",
                                      name);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return nullptr;
    }
    if (rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return nullptr;
        }
        return nullptr;
    }

    const auto& row = rows[0];
    auto user_ptr = std::make_shared<UserInfo>();
    user_ptr->email = row["email"].is_null() ? "" : row["email"].c_str();
    user_ptr->name = row["name"].is_null() ? "" : row["name"].c_str();
    user_ptr->nick = row["nick"].is_null() ? "" : row["nick"].c_str();
    user_ptr->desc = row["desc"].is_null() ? "" : row["desc"].c_str();
    user_ptr->sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
    user_ptr->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
    user_ptr->user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
    user_ptr->uid = row["uid"].as<int>();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return nullptr;
    }
    return user_ptr;
}

bool PostgresDao::GetUidByUserId(const std::string& user_id, int& uid)
{
    uid = 0;
    if (!IsValidUserPublicId(user_id))
    {
        return false;
    }

    pqxx::connection conn(account_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params("SELECT uid FROM \"user\" WHERE user_id = $1 LIMIT 1", user_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return false;
    }
    uid = rows[0]["uid"].as<int>();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return postgres_dao_users_modules::HasPositiveUid(uid);
}

bool PostgresDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    for (const auto& row : rows)
    {
        apply_rows.push_back({row["from_uid"].as<int>(), row["status"].is_null() ? 0 : row["status"].as<int>()});
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
        auto apply_ptr = std::make_shared<ApplyInfo>(ar.uid, name, "", icon, nick, sex, ar.status, user_public_id);
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
            txn.exec("SELECT from_uid, tag FROM friend_apply_tag WHERE to_uid = $1 AND from_uid IN (" + in_clause +
                         ") ORDER BY id ASC",
                     tag_params);
        if (!tag_rows.ok())
        {
            const auto& postgres_error = tag_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

std::unordered_map<int, std::shared_ptr<UserInfo>> PostgresDao::GetUsersByUids(const std::vector<int>& uids)
{
    std::unordered_map<int, std::shared_ptr<UserInfo>> result;
    if (uids.empty())
    {
        return result;
    }

    pqxx::connection conn(account_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "GetUsersByUids failed: " << postgres_error << std::endl;
        return result;
    }
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
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "GetUsersByUids failed: " << postgres_error << std::endl;
        return result;
    }
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "GetUsersByUids failed: " << postgres_error << std::endl;
        return result;
    }

    return result;
}

bool PostgresDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    std::vector<int> friend_ids;
    std::unordered_map<int, std::shared_ptr<UserInfo>> friend_by_id;
    std::unordered_map<int, std::string> back_by_id;
    // Step 1: relation rows (friend ids + back name) — no JOIN to "user".
    const auto rows = txn.exec_params("SELECT f.friend_id, f.back FROM friend AS f WHERE f.self_id = $1", self_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
        const auto tag_rows = txn.exec("SELECT friend_id, tag FROM friend_tag WHERE self_id = $1 AND friend_id IN (" +
                                           in_clause + ") ORDER BY id ASC",
                                       tag_params);
        if (!tag_rows.ok())
        {
            const auto& postgres_error = tag_rows.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::IsFriend(const int& self_id, const int& friend_id)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("SELECT 1 FROM friend WHERE self_id = $1 AND friend_id = $2 LIMIT 1", self_id, friend_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return !rows.empty();
}

std::vector<int> PostgresDao::FilterFriendUids(int viewer_uid, const std::vector<int>& author_uids)
{
    // Returns the subset of author_uids visible to viewer_uid under the moments
    // "friends-only" rule: a bidirectional `friend` row OR an accepted (status=1)
    // friend_apply in either direction. Mirrors the EXISTS logic the moments feed
    // used to embed inline, so visibility semantics are unchanged after the feed
    // query stops touching the friend tables directly.
    std::vector<int> result;
    if (viewer_uid <= 0 || author_uids.empty())
    {
        return result;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "FilterFriendUids PostgreSQL error: " << postgres_error << std::endl;
        result.clear();
        return result;
    }
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
    const auto rows =
        txn.exec("SELECT a.uid FROM (SELECT unnest(ARRAY[" + in_clause +
                     "]::int[]) AS uid) AS a "
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
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "FilterFriendUids PostgreSQL error: " << postgres_error << std::endl;
        result.clear();
        return result;
    }
    result.reserve(rows.size());
    for (const auto& row : rows)
    {
        result.push_back(row["uid"].as<int>());
    }
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "FilterFriendUids PostgreSQL error: " << postgres_error << std::endl;
        result.clear();
        return result;
    }

    return result;
}
