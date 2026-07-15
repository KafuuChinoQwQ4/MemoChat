#include "PostgresDao.hpp"
#include "ChatAccountDirectory.hpp"
#include "ConfigMgr.hpp"
#include "PostgresDaoUtil.hpp"
#include "db/PqxxCompat.hpp"
#include "SnowflakeUtil.hpp"
#include <set>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <random>
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

    const int final_limit = postgres_dao_groups_modules::ClampMemberLimit(member_limit);
    std::unordered_set<int> member_set;
    for (int uid : initial_members)
    {
        if (postgres_dao_groups_modules::ShouldKeepInitialMember(uid, owner_uid))
        {
            member_set.insert(uid);
        }
    }
    if (!postgres_dao_groups_modules::IsMemberCountWithinLimit(static_cast<int>(member_set.size()) + 1, final_limit))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    std::string group_code;
    for (int i = 0; i < 20; ++i)
    {
        const std::string candidate = GenerateGroupCode();
        const auto dup = txn.exec_params("SELECT 1 FROM chat_group WHERE group_code = $1 LIMIT 1", candidate);
        if (!dup.ok())
        {
            const auto& postgres_error = dup.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        if (dup.empty())
        {
            group_code = candidate;
            break;
        }
    }
    if (group_code.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
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
    if (!group_rows.ok())
    {
        const auto& postgres_error = group_rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (group_rows.empty())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    for (int uid : member_set)
    {
        txn.exec_params0("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
                         "VALUES($1,$2,1,0,1,1) "
                         "ON CONFLICT (group_id, uid) DO UPDATE SET status = 1, role = 1, mute_until = 0",
                         out_group_id,
                         uid);
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
    out_group_code = group_code;
    return true;
}

bool PostgresDao::GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id)
{
    out_group_id = 0;
    if (!IsValidGroupCode(group_code))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("SELECT group_id FROM chat_group WHERE group_code = $1 AND status = 1 LIMIT 1", group_code);
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
    out_group_id = rows[0]["group_id"].as<int64_t>();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return postgres_dao_groups_modules::HasPositiveGroupId(out_group_id);
}

bool PostgresDao::GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetGroupMemberList(const int64_t& group_id,
                                     std::vector<std::shared_ptr<GroupMemberInfo>>& member_list)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    // Step 1: membership rows only — no JOIN to "user".
    const auto rows = txn.exec_params("SELECT m.group_id, m.uid, m.role, m.mute_until, m.status "
                                      "FROM chat_group_member m "
                                      "WHERE m.group_id = $1 AND m.status = 1 ORDER BY m.role DESC, m.id ASC",
                                      group_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
    // Step 2: batch user base-info via the account isolation seam (finding #3).
    auto users = AccountDirectory().GetManyByUids(member_uids);
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
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

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, reason, "
                    "reviewer_uid) "
                    "VALUES($1, $2, $3, 1, 0, $4, 0)",
                    group_id,
                    target_uid,
                    inviter_uid,
                    reason);
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

bool PostgresDao::ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason)
{
    if (IsUserInGroup(group_id, applicant_uid))
    {
        return true;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group_apply SET reason = $1, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $2 AND applicant_uid = $3 AND type = 2 AND status = 0",
                                         reason,
                                         group_id,
                                         applicant_uid);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (updated.affected_rows() <= 0)
    {
        txn.exec_params("INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, "
                        "reason, reviewer_uid) "
                        "VALUES($1, $2, 0, 2, 0, $3, 0)",
                        group_id,
                        applicant_uid,
                        reason);
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

bool PostgresDao::ReviewGroupApply(const int64_t& apply_id,
                                   const int& reviewer_uid,
                                   const bool& agree,
                                   std::shared_ptr<GroupApplyInfo>& apply_info)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params("SELECT apply_id, group_id, applicant_uid, inviter_uid, type, status, reason "
                                      "FROM chat_group_apply WHERE apply_id = $1 LIMIT 1",
                                      apply_id);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
        if (!txn.commit())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return true;
    }

    const auto updated =
        txn.exec_params("UPDATE chat_group_apply SET status = $1, reviewer_uid = $2, updated_at = CURRENT_TIMESTAMP "
                        "WHERE apply_id = $3",
                        postgres_dao_groups_modules::ReviewedApplyStatus(agree),
                        reviewer_uid,
                        apply_id);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2",
                        found->group_id,
                        found->applicant_uid);
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
    found->status = postgres_dao_groups_modules::ReviewedApplyStatus(agree);
    apply_info = found;
    return true;
}

bool PostgresDao::UpdateGroupAnnouncement(const int64_t& group_id,
                                          const int& operator_uid,
                                          const std::string& announcement)
{
    if (!HasGroupPermission(group_id, operator_uid, kPermChangeGroupInfo))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group SET announcement = $1, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $2 AND status = 1",
                                         announcement,
                                         group_id);
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
    return postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows());
}

bool PostgresDao::UpdateGroupIcon(const int64_t& group_id, const int& operator_uid, const std::string& icon)
{
    if (!HasGroupPermission(group_id, operator_uid, kPermChangeGroupInfo))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group SET icon = $1, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $2 AND status = 1",
                                         icon,
                                         group_id);
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
    return postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows());
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

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group_member SET role = $1, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $2 AND uid = $3 AND status = 1",
                                         is_admin ? 2 : 1,
                                         group_id,
                                         target_uid);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
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
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
    }
    else
    {
        txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2",
                        group_id,
                        target_uid);
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

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group_member SET mute_until = $1, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $2 AND uid = $3 AND status = 1",
                                         postgres_dao_groups_modules::ClampMuteUntil(mute_until),
                                         group_id,
                                         target_uid);
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
    return postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows());
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

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group_member SET status = 3, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $1 AND uid = $2 AND status = 1",
                                         group_id,
                                         target_uid);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
    {
        txn.abort();

        return false;
    }
    txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2", group_id, target_uid);
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

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group_member SET status = 2, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $1 AND uid = $2 AND status = 1",
                                         group_id,
                                         uid);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
    {
        txn.abort();

        return false;
    }
    txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1 AND uid = $2", group_id, uid);
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

bool PostgresDao::DissolveGroup(const int64_t& group_id, const int& operator_uid)
{
    int role = 0;
    if (!GetUserRoleInGroup(group_id, operator_uid, role) || !postgres_dao_groups_modules::CanDissolveGroupRole(role))
    {
        return false;
    }

    pqxx::connection conn(postgres_connection_string_);
    pqxx::work txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto updated = txn.exec_params("UPDATE chat_group SET status = 2, updated_at = CURRENT_TIMESTAMP "
                                         "WHERE group_id = $1 AND owner_uid = $2 AND status = 1",
                                         group_id,
                                         operator_uid);
    if (!updated.ok())
    {
        const auto& postgres_error = updated.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (!postgres_dao_groups_modules::HasAffectedRows(updated.affected_rows()))
    {
        txn.abort();

        return false;
    }
    txn.exec_params("UPDATE chat_group_member SET status = 4, updated_at = CURRENT_TIMESTAMP "
                    "WHERE group_id = $1 AND status = 1",
                    group_id);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    txn.exec_params("DELETE FROM chat_group_admin_permission WHERE group_id = $1", group_id);
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

bool PostgresDao::GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info)
{
    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params(
        "SELECT group_id, group_code, name, icon, owner_uid, announcement, member_limit, is_all_muted, status "
        "FROM chat_group WHERE group_id = $1 LIMIT 1",
        group_id);
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
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
}

bool PostgresDao::GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role)
{
    role = 0;

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows =
        txn.exec_params("SELECT role FROM chat_group_member WHERE group_id = $1 AND uid = $2 AND status = 1 LIMIT 1",
                        group_id,
                        uid);
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
    role = rows[0]["role"].as<int>();
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
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

    pqxx::connection conn(postgres_connection_string_);
    pqxx::read_transaction txn(conn);
    if (!conn.is_open() || !txn.ok())
    {
        const auto& postgres_error = conn.is_open() ? txn.error_message() : conn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    const auto rows = txn.exec_params("SELECT permission_bits FROM chat_group_admin_permission "
                                      "WHERE group_id = $1 AND uid = $2 LIMIT 1",
                                      group_id,
                                      uid);
    if (!rows.ok())
    {
        const auto& postgres_error = rows.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    if (rows.empty() || rows[0]["permission_bits"].is_null())
    {
        if (!txn.ok())
        {
            const auto& postgres_error = txn.error_message();
            std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
            return false;
        }
        return true;
    }
    out_bits = postgres_dao_groups_modules::NormalizeStoredPermissionBits(rows[0]["permission_bits"].as<int64_t>(),
                                                                          kDefaultAdminPermBits);
    if (!txn.ok())
    {
        const auto& postgres_error = txn.error_message();
        std::cerr << "PostgreSQL error: " << postgres_error << std::endl;
        return false;
    }
    return true;
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
