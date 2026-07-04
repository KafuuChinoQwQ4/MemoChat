#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "PostgresDaoUtil.hpp"
#include "db/PqxxCompat.hpp"
#include "PostgresPool.hpp"
#include "SnowflakeUtil.hpp"
#include <pqxx/pqxx>
#include <set>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <stdexcept>
#include <limits>
#include <sstream>

import memochat.chat.postgres_dao_groups_algorithms;

namespace postgres_dao_groups_modules = memochat::chat::persistence::postgres_dao_groups::modules;

namespace
{
bool IsValidGroupCode(const std::string& group_code)
{
    const int length = static_cast<int>(group_code.size());
    const char prefix = length > 0 ? group_code[0] : '\0';
    const char first_digit = length > 1 ? group_code[1] : '\0';
    if (!postgres_dao_groups_modules::HasGroupCodeHeader(length, prefix, first_digit))
    {
        return false;
    }
    for (size_t i = 2; i < group_code.size(); ++i)
    {
        if (!postgres_dao_groups_modules::IsGroupCodeTailChar(group_code[i]))
        {
            return false;
        }
    }
    return true;
}

void ExecuteIgnoreSql(sql::Statement* stmt, const std::string& sql_text)
{
    if (stmt == nullptr)
    {
        return;
    }
    try
    {
        stmt->execute(sql_text);
    }
    catch (const sql::SQLException&)
    {
    }
}
} // namespace
bool PostgresDao::CreateGroup(const int& owner_uid,
                              const std::string& name,
                              const std::string& announcement,
                              const int& member_limit,
                              const std::vector<int>& initial_members,
                              int64_t& out_group_id,
                              std::string& out_group_code)
{
    out_group_id = 0;
    out_group_code.clear();
    if (!postgres_dao_groups_modules::CanCreateGroup(owner_uid, name.empty()))
    {
        return false;
    }

    if (use_postgres_)
    {
        try
        {
            const int final_limit = postgres_dao_groups_modules::ClampMemberLimit(member_limit);
            std::unordered_set<int> member_set;
            for (int uid : initial_members)
            {
                if (postgres_dao_groups_modules::ShouldKeepInitialMember(uid, owner_uid))
                {
                    member_set.insert(uid);
                }
            }
            if (!postgres_dao_groups_modules::IsMemberCountWithinLimit(static_cast<int>(member_set.size()) + 1,
                                                                       final_limit))
            {
                return false;
            }

            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            std::string group_code;
            for (int i = 0; i < 20; ++i)
            {
                const std::string candidate = GenerateGroupCode();
                const auto dup = txn.exec_params("SELECT 1 FROM chat_group WHERE group_code = $1 LIMIT 1", candidate);
                if (dup.empty())
                {
                    group_code = candidate;
                    break;
                }
            }
            if (group_code.empty())
            {
                return false;
            }

            const auto group_rows = txn.exec_params(
                "INSERT INTO chat_group(group_code, name, owner_uid, announcement, member_limit, is_all_muted, status) "
                "VALUES($1,$2,$3,$4,$5,false,1) RETURNING group_id",
                group_code,
                name,
                owner_uid,
                announcement,
                final_limit);
            if (group_rows.empty())
            {
                return false;
            }
            out_group_id = group_rows[0]["group_id"].as<int64_t>();
            if (!postgres_dao_groups_modules::HasValidGeneratedGroupId(out_group_id))
            {
                txn.abort();
                return false;
            }

            txn.exec_params0("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                             "VALUES($1,$2,3,0,0,1)",
                             out_group_id,
                             owner_uid);
            for (int uid : member_set)
            {
                txn.exec_params0("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                                 "VALUES($1,$2,1,0,1,1) "
                                 "ON CONFLICT (group_id, uid) DO UPDATE SET status = 1, role = 1, mute_until = 0",
                                 out_group_id,
                                 uid);
            }
            txn.commit();
            out_group_code = group_code;
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
        const int final_limit = postgres_dao_groups_modules::ClampMemberLimit(member_limit);
        std::unordered_set<int> member_set;
        for (int uid : initial_members)
        {
            if (postgres_dao_groups_modules::ShouldKeepInitialMember(uid, owner_uid))
            {
                member_set.insert(uid);
            }
        }
        if (!postgres_dao_groups_modules::IsMemberCountWithinLimit(static_cast<int>(member_set.size()) + 1,
                                                                   final_limit))
        {
            return false;
        }

        con->_con->setAutoCommit(false);

        std::string group_code;
        for (int i = 0; i < 20; ++i)
        {
            const std::string candidate = GenerateGroupCode();
            std::unique_ptr<sql::PreparedStatement> check_group_code(
                con->_con->prepareStatement("SELECT 1 FROM chat_group WHERE group_code = ? LIMIT 1"));
            check_group_code->setString(1, candidate);
            std::unique_ptr<sql::ResultSet> check_group_code_res(check_group_code->executeQuery());
            if (!check_group_code_res->next())
            {
                group_code = candidate;
                break;
            }
        }
        if (group_code.empty())
        {
            con->_con->rollback();
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> ins_group(con->_con->prepareStatement(
            "INSERT INTO chat_group(group_code, name, owner_uid, announcement, member_limit, is_all_muted, status) "
            "VALUES(?,?,?,?,?,0,1)"));
        ins_group->setString(1, group_code);
        ins_group->setString(2, name);
        ins_group->setInt(3, owner_uid);
        ins_group->setString(4, announcement);
        ins_group->setInt(5, final_limit);
        if (!postgres_dao_groups_modules::HasAffectedRows(ins_group->executeUpdate()))
        {
            con->_con->rollback();
            return false;
        }

        std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> id_res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS group_id"));
        if (!id_res->next())
        {
            con->_con->rollback();
            return false;
        }
        out_group_id = static_cast<int64_t>(std::stoll(id_res->getString("group_id")));
        if (!postgres_dao_groups_modules::HasValidGeneratedGroupId(out_group_id))
        {
            con->_con->rollback();
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> ins_member(con->_con->prepareStatement(
            "INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
            "VALUES(?,?,?,?,?,1)"));

        ins_member->setInt64(1, out_group_id);
        ins_member->setInt(2, owner_uid);
        ins_member->setInt(3, 3);
        ins_member->setInt64(4, 0);
        ins_member->setInt(5, 0);
        if (!postgres_dao_groups_modules::HasAffectedRows(ins_member->executeUpdate()))
        {
            con->_con->rollback();
            return false;
        }

        for (int uid : member_set)
        {
            ins_member->setInt64(1, out_group_id);
            ins_member->setInt(2, uid);
            ins_member->setInt(3, 1);
            ins_member->setInt64(4, 0);
            ins_member->setInt(5, 1);
            ins_member->executeUpdate();
        }

        con->_con->commit();
        out_group_code = group_code;
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

bool PostgresDao::GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id)
{
    out_group_id = 0;
    if (!IsValidGroupCode(group_code))
    {
        return false;
    }

    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows =
                txn.exec_params("SELECT group_id FROM chat_group WHERE group_code = $1 AND status = 1 LIMIT 1",
                                group_code);
            if (rows.empty())
            {
                return false;
            }
            out_group_id = rows[0]["group_id"].as<int64_t>();
            return postgres_dao_groups_modules::HasPositiveGroupId(out_group_id);
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
            con->_con->prepareStatement("SELECT group_id FROM chat_group WHERE group_code = ? AND status = 1 LIMIT 1"));
        pstmt->setString(1, group_code);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return false;
        }
        out_group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
        return postgres_dao_groups_modules::HasPositiveGroupId(out_group_id);
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params(
                "SELECT g.group_id, g.group_code, g.name, g.icon, g.owner_uid, g.announcement, g.member_limit, "
                "g.is_all_muted, g.status, m.role, "
                "CASE "
                "WHEN m.role >= 3 THEN $1 "
                "WHEN m.role = 2 THEN COALESCE(NULLIF(p.permission_bits, 0), $2) "
                "ELSE 0 END AS permission_bits, "
                "(SELECT COUNT(1) FROM chat_group_member gm WHERE gm.group_id = g.group_id AND gm.status = 1) AS "
                "member_count "
                "FROM chat_group_member m JOIN chat_group g ON m.group_id = g.group_id "
                "LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
                "WHERE m.uid = $3 AND m.status = 1 AND g.status = 1 ORDER BY g.updated_at DESC",
                kOwnerPermBits,
                kDefaultAdminPermBits,
                uid);
            for (const auto& row : rows)
            {
                auto info = std::make_shared<GroupInfo>();
                info->group_id = row["group_id"].as<int64_t>();
                info->group_code = row["group_code"].is_null() ? "" : row["group_code"].c_str();
                info->name = row["name"].is_null() ? "" : row["name"].c_str();
                info->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
                info->owner_uid = row["owner_uid"].is_null() ? 0 : row["owner_uid"].as<int>();
                info->announcement = row["announcement"].is_null() ? "" : row["announcement"].c_str();
                info->member_limit = row["member_limit"].is_null() ? 0 : row["member_limit"].as<int>();
                info->is_all_muted = row["is_all_muted"].is_null() ? 0 : row["is_all_muted"].as<bool>();
                info->status = row["status"].is_null() ? 0 : row["status"].as<int>();
                info->role = row["role"].is_null() ? 0 : row["role"].as<int>();
                info->permission_bits = row["permission_bits"].is_null() ? 0 : row["permission_bits"].as<int64_t>();
                info->member_count = row["member_count"].is_null() ? 0 : row["member_count"].as<int>();
                group_list.push_back(info);
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
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT g.group_id, g.group_code, g.name, g.icon, g.owner_uid, g.announcement, g.member_limit, "
            "g.is_all_muted, g.status, m.role, "
            "CASE "
            "WHEN m.role >= 3 THEN ? "
            "WHEN m.role = 2 THEN COALESCE(NULLIF(p.permission_bits, 0), ?) "
            "ELSE 0 END AS permission_bits, "
            "(SELECT COUNT(1) FROM chat_group_member gm WHERE gm.group_id = g.group_id AND gm.status = 1) AS "
            "member_count "
            "FROM chat_group_member m JOIN chat_group g ON m.group_id = g.group_id "
            "LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
            "WHERE m.uid = ? AND m.status = 1 AND g.status = 1 ORDER BY g.updated_at DESC"));
        pstmt->setInt64(1, kOwnerPermBits);
        pstmt->setInt64(2, kDefaultAdminPermBits);
        pstmt->setInt(3, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            auto info = std::make_shared<GroupInfo>();
            info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
            info->group_code = res->isNull("group_code") ? "" : res->getString("group_code");
            info->name = res->getString("name");
            info->icon = res->isNull("icon") ? "" : res->getString("icon");
            info->owner_uid = res->getInt("owner_uid");
            info->announcement = res->getString("announcement");
            info->member_limit = res->getInt("member_limit");
            info->is_all_muted = res->getInt("is_all_muted");
            info->status = res->getInt("status");
            info->role = res->getInt("role");
            info->permission_bits = static_cast<int64_t>(std::stoll(res->getString("permission_bits")));
            info->member_count = res->getInt("member_count");
            group_list.push_back(info);
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

bool PostgresDao::GetGroupMemberList(const int64_t& group_id,
                                     std::vector<std::shared_ptr<GroupMemberInfo>>& member_list)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            // Step 1: membership rows only — no JOIN to "user".
            const auto rows = txn.exec_params("SELECT m.group_id, m.uid, m.role, m.mute_until, m.status "
                                              "FROM chat_group_member m "
                                              "WHERE m.group_id = $1 AND m.status = 1 ORDER BY m.role DESC, m.id ASC",
                                              group_id);
            std::vector<int> member_uids;
            for (const auto& row : rows)
            {
                auto info = std::make_shared<GroupMemberInfo>();
                info->group_id = row["group_id"].as<int64_t>();
                info->uid = row["uid"].is_null() ? 0 : row["uid"].as<int>();
                info->role = row["role"].is_null() ? 0 : row["role"].as<int>();
                info->mute_until = row["mute_until"].is_null() ? 0 : row["mute_until"].as<int64_t>();
                info->status = row["status"].is_null() ? 0 : row["status"].as<int>();
                member_list.push_back(info);
                if (info->uid != 0)
                {
                    member_uids.push_back(info->uid);
                }
            }
            // Step 2: batch user base-info (account-data seam, replaces JOIN).
            auto users = GetUsersByUids(member_uids);
            for (auto& info : member_list)
            {
                const auto uit = users.find(info->uid);
                if (uit != users.end() && uit->second)
                {
                    info->name = uit->second->name;
                    info->nick = uit->second->nick;
                    info->icon = uit->second->icon;
                    info->user_id = uit->second->user_id;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT m.group_id, m.uid, m.role, m.mute_until, m.status, u.name, u.nick, u.icon, u.user_id "
            "FROM chat_group_member m LEFT JOIN user u ON m.uid = u.uid "
            "WHERE m.group_id = ? AND m.status = 1 ORDER BY m.role DESC, m.id ASC"));
        pstmt->setInt64(1, group_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            auto info = std::make_shared<GroupMemberInfo>();
            info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
            info->uid = res->getInt("uid");
            info->role = res->getInt("role");
            info->mute_until = static_cast<int64_t>(std::stoll(res->getString("mute_until")));
            info->status = res->getInt("status");
            info->name = res->isNull("name") ? "" : res->getString("name");
            info->nick = res->isNull("nick") ? "" : res->getString("nick");
            info->icon = res->isNull("icon") ? "" : res->getString("icon");
            info->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
            member_list.push_back(info);
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

bool PostgresDao::InviteGroupMember(const int64_t& group_id,
                                    const int& inviter_uid,
                                    const int& target_uid,
                                    const std::string& reason)
{
    if (!HasGroupPermission(group_id, inviter_uid, kPermInviteUsers))
    {
        return false;
    }
    if (!IsFriend(inviter_uid, target_uid))
    {
        return false;
    }
    if (IsUserInGroup(group_id, target_uid))
    {
        return true;
    }

    std::shared_ptr<GroupInfo> group_info;
    if (!GetGroupById(group_id, group_info) || !group_info)
    {
        return false;
    }
    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    if (!GetGroupMemberList(group_id, members))
    {
        return false;
    }
    if (!postgres_dao_groups_modules::HasRoomForMember(static_cast<int>(members.size()), group_info->member_limit))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            txn.exec_params("INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, reason, "
                            "reviewer_uid) "
                            "VALUES($1, $2, $3, 1, 0, $4, 0)",
                            group_id,
                            target_uid,
                            inviter_uid,
                            reason);
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
            "INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, reason, reviewer_uid) "
            "VALUES(?,?,?,1,0,?,0)"));
        pstmt->setInt64(1, group_id);
        pstmt->setInt(2, target_uid);
        pstmt->setInt(3, inviter_uid);
        pstmt->setString(4, reason);
        return pstmt->executeUpdate() >= 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason)
{
    if (IsUserInGroup(group_id, applicant_uid))
    {
        return true;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_group_apply SET reason = $1, updated_at = CURRENT_TIMESTAMP "
                                "WHERE group_id = $2 AND applicant_uid = $3 AND type = 2 AND status = 0",
                                reason,
                                group_id,
                                applicant_uid);
            if (updated.affected_rows() <= 0)
            {
                txn.exec_params("INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, "
                                "reason, reviewer_uid) "
                                "VALUES($1, $2, 0, 2, 0, $3, 0)",
                                group_id,
                                applicant_uid,
                                reason);
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
        std::unique_ptr<sql::PreparedStatement> upd(
            con->_con->prepareStatement("UPDATE chat_group_apply SET reason = ?, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND applicant_uid = ? AND type = 2 AND status = 0"));
        upd->setString(1, reason);
        upd->setInt64(2, group_id);
        upd->setInt(3, applicant_uid);
        if (upd->executeUpdate() > 0)
        {
            return true;
        }
        std::unique_ptr<sql::PreparedStatement> ins(con->_con->prepareStatement(
            "INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, reason, reviewer_uid) "
            "VALUES(?,?,0,2,0,?,0)"));
        ins->setInt64(1, group_id);
        ins->setInt(2, applicant_uid);
        ins->setString(3, reason);
        return ins->executeUpdate() >= 0;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::ReviewGroupApply(const int64_t& apply_id,
                                   const int& reviewer_uid,
                                   const bool& agree,
                                   std::shared_ptr<GroupApplyInfo>& apply_info)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto rows =
                txn.exec_params("SELECT apply_id, group_id, applicant_uid, inviter_uid, type, status, reason "
                                "FROM chat_group_apply WHERE apply_id = $1 LIMIT 1",
                                apply_id);
            if (rows.empty())
            {
                txn.abort();
                return false;
            }

            const auto& row = rows[0];
            auto found = std::make_shared<GroupApplyInfo>();
            found->apply_id = row["apply_id"].as<int64_t>();
            found->group_id = row["group_id"].as<int64_t>();
            found->applicant_uid = row["applicant_uid"].is_null() ? 0 : row["applicant_uid"].as<int>();
            found->inviter_uid = row["inviter_uid"].is_null() ? 0 : row["inviter_uid"].as<int>();
            found->type = row["type"].is_null() ? "apply" : ((row["type"].as<int>() == 1) ? "invite" : "apply");
            found->status = row["status"].is_null() ? 0 : row["status"].as<int>();
            found->reason = row["reason"].is_null() ? "" : row["reason"].c_str();
            found->reviewer_uid = reviewer_uid;

            if (!HasGroupPermission(found->group_id, reviewer_uid, kPermInviteUsers))
            {
                txn.abort();
                return false;
            }

            if (!postgres_dao_groups_modules::IsPendingApplyStatus(found->status))
            {
                apply_info = found;
                txn.commit();
                return true;
            }

            const auto updated = txn.exec_params(
                "UPDATE chat_group_apply SET status = $1, reviewer_uid = $2, updated_at = CURRENT_TIMESTAMP "
                "WHERE apply_id = $3",
                postgres_dao_groups_modules::ReviewedApplyStatus(agree),
                reviewer_uid,
                apply_id);
            if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
            {
                txn.abort();
                return false;
            }

            if (agree && !IsUserInGroup(found->group_id, found->applicant_uid))
            {
                txn.exec_params("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                                "VALUES($1, $2, 1, 0, $3, 1) "
                                "ON CONFLICT (group_id, uid) DO UPDATE SET "
                                "status = 1, role = 1, mute_until = 0, updated_at = CURRENT_TIMESTAMP",
                                found->group_id,
                                found->applicant_uid,
                                postgres_dao_groups_modules::JoinSourceForApplyType(found->type == "invite"));
                txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2",
                                found->group_id,
                                found->applicant_uid);
            }

            txn.commit();
            found->status = postgres_dao_groups_modules::ReviewedApplyStatus(agree);
            apply_info = found;
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
        std::unique_ptr<sql::PreparedStatement> query(
            con->_con->prepareStatement("SELECT apply_id, group_id, applicant_uid, inviter_uid, type, status, reason "
                                        "FROM chat_group_apply WHERE apply_id = ? LIMIT 1"));
        query->setInt64(1, apply_id);
        std::unique_ptr<sql::ResultSet> res(query->executeQuery());
        if (!res->next())
        {
            con->_con->rollback();
            return false;
        }

        auto found = std::make_shared<GroupApplyInfo>();
        found->apply_id = static_cast<int64_t>(std::stoll(res->getString("apply_id")));
        found->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
        found->applicant_uid = res->getInt("applicant_uid");
        found->inviter_uid = res->getInt("inviter_uid");
        found->type = (res->getInt("type") == 1) ? "invite" : "apply";
        found->status = res->getInt("status");
        found->reason = res->getString("reason");
        found->reviewer_uid = reviewer_uid;

        if (!HasGroupPermission(found->group_id, reviewer_uid, kPermInviteUsers))
        {
            con->_con->rollback();
            return false;
        }

        if (!postgres_dao_groups_modules::IsPendingApplyStatus(found->status))
        {
            apply_info = found;
            con->_con->commit();
            return true;
        }

        std::unique_ptr<sql::PreparedStatement> upd(con->_con->prepareStatement(
            "UPDATE chat_group_apply SET status = ?, reviewer_uid = ?, updated_at = CURRENT_TIMESTAMP "
            "WHERE apply_id = ?"));
        upd->setInt(1, postgres_dao_groups_modules::ReviewedApplyStatus(agree));
        upd->setInt(2, reviewer_uid);
        upd->setInt64(3, apply_id);
        if (!postgres_dao_groups_modules::HasAffectedRows(upd->executeUpdate()))
        {
            con->_con->rollback();
            return false;
        }

        if (agree && !IsUserInGroup(found->group_id, found->applicant_uid))
        {
            std::unique_ptr<sql::PreparedStatement> ins_member(con->_con->prepareStatement(
                "INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                "VALUES(?,?,1,0,?,1) ON DUPLICATE KEY UPDATE status = 1, role = 1, mute_until = 0, updated_at = "
                "CURRENT_TIMESTAMP"));
            ins_member->setInt64(1, found->group_id);
            ins_member->setInt(2, found->applicant_uid);
            ins_member->setInt(3, postgres_dao_groups_modules::JoinSourceForApplyType(found->type == "invite"));
            ins_member->executeUpdate();
            std::unique_ptr<sql::PreparedStatement> del_perm(
                con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
            del_perm->setInt64(1, found->group_id);
            del_perm->setInt(2, found->applicant_uid);
            del_perm->executeUpdate();
        }

        con->_con->commit();
        found->status = postgres_dao_groups_modules::ReviewedApplyStatus(agree);
        apply_info = found;
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

bool PostgresDao::UpdateGroupAnnouncement(const int64_t& group_id,
                                          const int& operator_uid,
                                          const std::string& announcement)
{
    if (!HasGroupPermission(group_id, operator_uid, kPermChangeGroupInfo))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_group SET announcement = $1, updated_at = CURRENT_TIMESTAMP "
                                "WHERE group_id = $2 AND status = 1",
                                announcement,
                                group_id);
            txn.commit();
            return postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows());
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
            con->_con->prepareStatement("UPDATE chat_group SET announcement = ?, updated_at = CURRENT_TIMESTAMP WHERE "
                                        "group_id = ? AND status = 1"));
        pstmt->setString(1, announcement);
        pstmt->setInt64(2, group_id);
        return postgres_dao_groups_modules::HasAffectedRows(pstmt->executeUpdate());
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::UpdateGroupIcon(const int64_t& group_id, const int& operator_uid, const std::string& icon)
{
    if (!HasGroupPermission(group_id, operator_uid, kPermChangeGroupInfo))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated = txn.exec_params("UPDATE chat_group SET icon = $1, updated_at = CURRENT_TIMESTAMP "
                                                 "WHERE group_id = $2 AND status = 1",
                                                 icon,
                                                 group_id);
            txn.commit();
            return postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows());
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
            "UPDATE chat_group SET icon = ?, updated_at = CURRENT_TIMESTAMP WHERE group_id = ? AND status = 1"));
        pstmt->setString(1, icon);
        pstmt->setInt64(2, group_id);
        return postgres_dao_groups_modules::HasAffectedRows(pstmt->executeUpdate());
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::SetGroupAdmin(const int64_t& group_id,
                                const int& operator_uid,
                                const int& target_uid,
                                const bool& is_admin,
                                const int64_t& permission_bits)
{
    if (!HasGroupPermission(group_id, operator_uid, kPermManageAdmins))
    {
        return false;
    }
    int operator_role = 0;
    if (!GetUserRoleInGroup(group_id, operator_uid, operator_role) ||
        !postgres_dao_groups_modules::CanOperatorManageAdmins(operator_role))
    {
        return false;
    }
    int target_role = 0;
    if (!GetUserRoleInGroup(group_id, target_uid, target_role) ||
        !postgres_dao_groups_modules::CanTargetBecomeAdmin(target_role))
    {
        return false;
    }
    if (!postgres_dao_groups_modules::CanOperatorChangeTargetRole(operator_role, target_role))
    {
        return false;
    }
    const int64_t normalized_perm_bits =
        postgres_dao_groups_modules::NormalizeAdminPermissionBitsForRole(is_admin,
                                                                         permission_bits,
                                                                         kOwnerPermBits,
                                                                         kDefaultAdminPermBits);
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_group_member SET role = $1, updated_at = CURRENT_TIMESTAMP "
                                "WHERE group_id = $2 AND uid = $3 AND status = 1",
                                is_admin ? 2 : 1,
                                group_id,
                                target_uid);
            if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
            {
                txn.abort();
                return false;
            }

            if (is_admin)
            {
                txn.exec_params("INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
                                "VALUES($1, $2, $3) "
                                "ON CONFLICT (group_id, uid) DO UPDATE SET "
                                "permission_bits = EXCLUDED.permission_bits, updated_at = CURRENT_TIMESTAMP",
                                group_id,
                                target_uid,
                                normalized_perm_bits);
            }
            else
            {
                txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2",
                                group_id,
                                target_uid);
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("UPDATE chat_group_member SET role = ?, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND uid = ? AND status = 1"));
        pstmt->setInt(1, is_admin ? 2 : 1);
        pstmt->setInt64(2, group_id);
        pstmt->setInt(3, target_uid);
        if (!postgres_dao_groups_modules::HasAffectedRows(pstmt->executeUpdate()))
        {
            con->_con->rollback();
            return false;
        }

        if (is_admin)
        {
            std::unique_ptr<sql::PreparedStatement> up_perm(
                con->_con->prepareStatement("INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
                                            "VALUES(?,?,?) ON DUPLICATE KEY UPDATE permission_bits = "
                                            "VALUES(permission_bits), updated_at = CURRENT_TIMESTAMP"));
            up_perm->setInt64(1, group_id);
            up_perm->setInt(2, target_uid);
            up_perm->setInt64(3, normalized_perm_bits);
            up_perm->executeUpdate();
        }
        else
        {
            std::unique_ptr<sql::PreparedStatement> del_perm(
                con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
            del_perm->setInt64(1, group_id);
            del_perm->setInt(2, target_uid);
            del_perm->executeUpdate();
        }
        con->_con->commit();
        return true;
    }
    catch (sql::SQLException& e)
    {
        try
        {
            con->_con->rollback();
        }
        catch (...)
        {
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::MuteGroupMember(const int64_t& group_id,
                                  const int& operator_uid,
                                  const int& target_uid,
                                  const int64_t& mute_until)
{
    int op_role = 0;
    if (!GetUserRoleInGroup(group_id, operator_uid, op_role) ||
        !postgres_dao_groups_modules::CanModerateOperatorRole(op_role) ||
        !HasGroupPermission(group_id, operator_uid, kPermBanUsers))
    {
        return false;
    }
    int target_role = 0;
    if (!GetUserRoleInGroup(group_id, target_uid, target_role) ||
        !postgres_dao_groups_modules::CanModerateTargetRole(op_role, target_role))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_group_member SET mute_until = $1, updated_at = CURRENT_TIMESTAMP "
                                "WHERE group_id = $2 AND uid = $3 AND status = 1",
                                postgres_dao_groups_modules::ClampMuteUntil(mute_until),
                                group_id,
                                target_uid);
            txn.commit();
            return postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows());
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
            con->_con->prepareStatement("UPDATE chat_group_member SET mute_until = ?, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND uid = ? AND status = 1"));
        pstmt->setInt64(1, postgres_dao_groups_modules::ClampMuteUntil(mute_until));
        pstmt->setInt64(2, group_id);
        pstmt->setInt(3, target_uid);
        return postgres_dao_groups_modules::HasAffectedRows(pstmt->executeUpdate());
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::KickGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid)
{
    int op_role = 0;
    if (!GetUserRoleInGroup(group_id, operator_uid, op_role) ||
        !postgres_dao_groups_modules::CanModerateOperatorRole(op_role) ||
        !HasGroupPermission(group_id, operator_uid, kPermBanUsers))
    {
        return false;
    }
    int target_role = 0;
    if (!GetUserRoleInGroup(group_id, target_uid, target_role) ||
        !postgres_dao_groups_modules::CanModerateTargetRole(op_role, target_role))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_group_member SET status = 3, updated_at = CURRENT_TIMESTAMP "
                                "WHERE group_id = $1 AND uid = $2 AND status = 1",
                                group_id,
                                target_uid);
            if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
            {
                txn.abort();
                return false;
            }
            txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2",
                            group_id,
                            target_uid);
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
            con->_con->prepareStatement("UPDATE chat_group_member SET status = 3, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND uid = ? AND status = 1"));
        pstmt->setInt64(1, group_id);
        pstmt->setInt(2, target_uid);
        if (!postgres_dao_groups_modules::HasAffectedRows(pstmt->executeUpdate()))
        {
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> del_perm(
            con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
        del_perm->setInt64(1, group_id);
        del_perm->setInt(2, target_uid);
        del_perm->executeUpdate();
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

bool PostgresDao::QuitGroup(const int64_t& group_id, const int& uid)
{
    int role = 0;
    if (!GetUserRoleInGroup(group_id, uid, role))
    {
        return false;
    }
    if (!postgres_dao_groups_modules::CanQuitGroupRole(role))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated =
                txn.exec_params("UPDATE chat_group_member SET status = 2, updated_at = CURRENT_TIMESTAMP "
                                "WHERE group_id = $1 AND uid = $2 AND status = 1",
                                group_id,
                                uid);
            if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
            {
                txn.abort();
                return false;
            }
            txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2", group_id, uid);
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
            con->_con->prepareStatement("UPDATE chat_group_member SET status = 2, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND uid = ? AND status = 1"));
        pstmt->setInt64(1, group_id);
        pstmt->setInt(2, uid);
        if (!postgres_dao_groups_modules::HasAffectedRows(pstmt->executeUpdate()))
        {
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> del_perm(
            con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
        del_perm->setInt64(1, group_id);
        del_perm->setInt(2, uid);
        del_perm->executeUpdate();
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

bool PostgresDao::DissolveGroup(const int64_t& group_id, const int& operator_uid)
{
    int role = 0;
    if (!GetUserRoleInGroup(group_id, operator_uid, role) || !postgres_dao_groups_modules::CanDissolveGroupRole(role))
    {
        return false;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::work txn(conn);
            const auto updated = txn.exec_params("UPDATE chat_group SET status = 2, updated_at = CURRENT_TIMESTAMP "
                                                 "WHERE group_id = $1 AND owner_uid = $2 AND status = 1",
                                                 group_id,
                                                 operator_uid);
            if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
            {
                txn.abort();
                return false;
            }
            txn.exec_params("UPDATE chat_group_member SET status = 4, updated_at = CURRENT_TIMESTAMP "
                            "WHERE group_id = $1 AND status = 1",
                            group_id);
            txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1", group_id);
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
        std::unique_ptr<sql::PreparedStatement> update_group(
            con->_con->prepareStatement("UPDATE chat_group SET status = 2, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND owner_uid = ? AND status = 1"));
        update_group->setInt64(1, group_id);
        update_group->setInt(2, operator_uid);
        if (!postgres_dao_groups_modules::HasAffectedRows(update_group->executeUpdate()))
        {
            con->_con->rollback();
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> update_members(
            con->_con->prepareStatement("UPDATE chat_group_member SET status = 4, updated_at = CURRENT_TIMESTAMP "
                                        "WHERE group_id = ? AND status = 1"));
        update_members->setInt64(1, group_id);
        update_members->executeUpdate();
        std::unique_ptr<sql::PreparedStatement> del_perm(
            con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ?"));
        del_perm->setInt64(1, group_id);
        del_perm->executeUpdate();
        con->_con->commit();
        return true;
    }
    catch (sql::SQLException& e)
    {
        if (con && con->_con)
        {
            try
            {
                con->_con->rollback();
            }
            catch (...)
            {
            }
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (legacy SQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool PostgresDao::GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info)
{
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params(
                "SELECT group_id, group_code, name, icon, owner_uid, announcement, member_limit, is_all_muted, status "
                "FROM chat_group WHERE group_id = $1 LIMIT 1",
                group_id);
            if (rows.empty())
            {
                return false;
            }
            const auto& row = rows[0];
            auto info = std::make_shared<GroupInfo>();
            info->group_id = row["group_id"].as<int64_t>();
            info->group_code = row["group_code"].is_null() ? "" : row["group_code"].c_str();
            info->name = row["name"].is_null() ? "" : row["name"].c_str();
            info->icon = row["icon"].is_null() ? "" : row["icon"].c_str();
            info->owner_uid = row["owner_uid"].is_null() ? 0 : row["owner_uid"].as<int>();
            info->announcement = row["announcement"].is_null() ? "" : row["announcement"].c_str();
            info->member_limit = row["member_limit"].is_null() ? 0 : row["member_limit"].as<int>();
            info->is_all_muted = row["is_all_muted"].is_null() ? 0 : row["is_all_muted"].as<bool>();
            info->status = row["status"].is_null() ? 0 : row["status"].as<int>();
            group_info = info;
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
            "SELECT group_id, group_code, name, icon, owner_uid, announcement, member_limit, is_all_muted, status "
            "FROM chat_group WHERE group_id = ? LIMIT 1"));
        pstmt->setInt64(1, group_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return false;
        }
        auto info = std::make_shared<GroupInfo>();
        info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
        info->group_code = res->isNull("group_code") ? "" : res->getString("group_code");
        info->name = res->getString("name");
        info->icon = res->isNull("icon") ? "" : res->getString("icon");
        info->owner_uid = res->getInt("owner_uid");
        info->announcement = res->getString("announcement");
        info->member_limit = res->getInt("member_limit");
        info->is_all_muted = res->getInt("is_all_muted");
        info->status = res->getInt("status");
        group_info = info;
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

bool PostgresDao::GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role)
{
    role = 0;
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params(
                "SELECT role FROM chat_group_member WHERE group_id = $1 AND uid = $2 AND status = 1 LIMIT 1",
                group_id,
                uid);
            if (rows.empty())
            {
                return false;
            }
            role = rows[0]["role"].as<int>();
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
            "SELECT role FROM chat_group_member WHERE group_id = ? AND uid = ? AND status = 1 LIMIT 1"));
        pstmt->setInt64(1, group_id);
        pstmt->setInt(2, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return false;
        }
        role = res->getInt("role");
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

bool PostgresDao::IsUserInGroup(const int64_t& group_id, const int& uid)
{
    int role = 0;
    return GetUserRoleInGroup(group_id, uid, role);
}

bool PostgresDao::GetGroupPermissionBits(const int64_t& group_id, const int& uid, int64_t& out_bits)
{
    out_bits = 0;
    int role = 0;
    if (!GetUserRoleInGroup(group_id, uid, role))
    {
        return false;
    }
    out_bits = postgres_dao_groups_modules::FallbackPermissionBitsForRole(role, kOwnerPermBits, kDefaultAdminPermBits);
    if (postgres_dao_groups_modules::IsOwnerRole(role) || postgres_dao_groups_modules::IsNormalMemberRole(role))
    {
        return true;
    }
    if (use_postgres_)
    {
        try
        {
            pqxx::connection conn(postgres_connection_string_);
            pqxx::read_transaction txn(conn);
            const auto rows = txn.exec_params("SELECT permission_bits FROM chat_group_admin_permission "
                                              "WHERE group_id = $1 AND uid = $2 LIMIT 1",
                                              group_id,
                                              uid);
            if (rows.empty() || rows[0]["permission_bits"].is_null())
            {
                return true;
            }
            out_bits =
                postgres_dao_groups_modules::NormalizeStoredPermissionBits(rows[0]["permission_bits"].as<int64_t>(),
                                                                           kDefaultAdminPermBits);
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
            con->_con->prepareStatement("SELECT permission_bits FROM chat_group_admin_permission "
                                        "WHERE group_id = ? AND uid = ? LIMIT 1"));
        pstmt->setInt64(1, group_id);
        pstmt->setInt(2, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return true;
        }
        out_bits = postgres_dao_groups_modules::NormalizeStoredPermissionBits(
            static_cast<int64_t>(std::stoll(res->getString("permission_bits"))),
            kDefaultAdminPermBits);
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

bool PostgresDao::HasGroupPermission(const int64_t& group_id, const int& uid, int64_t required_bits)
{
    int64_t bits = 0;
    if (!GetGroupPermissionBits(group_id, uid, bits))
    {
        return false;
    }
    return postgres_dao_groups_modules::HasRequiredPermissionBits(bits, required_bits);
}
