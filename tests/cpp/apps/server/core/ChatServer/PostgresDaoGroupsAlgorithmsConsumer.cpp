import memochat.chat.postgres_dao_groups_algorithms;

namespace postgres_dao_groups_modules = memochat::chat::persistence::postgres_dao_groups::modules;

bool MemoChatTestPostgresDaoGroupsCanCreate(int owner_uid, bool name_empty)
{
    return postgres_dao_groups_modules::CanCreateGroup(owner_uid, name_empty);
}

int MemoChatTestPostgresDaoGroupsClampLimit(int member_limit)
{
    return postgres_dao_groups_modules::ClampMemberLimit(member_limit);
}

bool MemoChatTestPostgresDaoGroupsKeepInitialMember(int uid, int owner_uid)
{
    return postgres_dao_groups_modules::ShouldKeepInitialMember(uid, owner_uid);
}

bool MemoChatTestPostgresDaoGroupsWithinLimit(int existing_member_count, int member_limit)
{
    return postgres_dao_groups_modules::IsMemberCountWithinLimit(existing_member_count, member_limit);
}

bool MemoChatTestPostgresDaoGroupsHasRoom(int current_member_count, int member_limit)
{
    return postgres_dao_groups_modules::HasRoomForMember(current_member_count, member_limit);
}

bool MemoChatTestPostgresDaoGroupsAffectedRows(long long affected_rows)
{
    return postgres_dao_groups_modules::HasAffectedRows(affected_rows);
}

bool MemoChatTestPostgresDaoGroupsCodeHeader(int length, char prefix, char first_digit)
{
    return postgres_dao_groups_modules::HasGroupCodeHeader(length, prefix, first_digit);
}

bool MemoChatTestPostgresDaoGroupsCodeTail(char c)
{
    return postgres_dao_groups_modules::IsGroupCodeTailChar(c);
}

bool MemoChatTestPostgresDaoGroupsPendingApply(int status)
{
    return postgres_dao_groups_modules::IsPendingApplyStatus(status);
}

int MemoChatTestPostgresDaoGroupsReviewedStatus(bool agree)
{
    return postgres_dao_groups_modules::ReviewedApplyStatus(agree);
}

int MemoChatTestPostgresDaoGroupsJoinSource(bool is_invite)
{
    return postgres_dao_groups_modules::JoinSourceForApplyType(is_invite);
}

bool MemoChatTestPostgresDaoGroupsCanManageAdmins(int operator_role)
{
    return postgres_dao_groups_modules::CanOperatorManageAdmins(operator_role);
}

bool MemoChatTestPostgresDaoGroupsCanTargetBecomeAdmin(int target_role)
{
    return postgres_dao_groups_modules::CanTargetBecomeAdmin(target_role);
}

bool MemoChatTestPostgresDaoGroupsCanChangeTarget(int operator_role, int target_role)
{
    return postgres_dao_groups_modules::CanOperatorChangeTargetRole(operator_role, target_role);
}

long long
MemoChatTestPostgresDaoGroupsNormalizeAdminPerm(long long permission_bits, long long owner_bits, long long default_bits)
{
    return postgres_dao_groups_modules::NormalizeAdminPermissionBits(permission_bits, owner_bits, default_bits);
}

long long MemoChatTestPostgresDaoGroupsNormalizeAdminPermForRole(bool is_admin,
                                                                 long long permission_bits,
                                                                 long long owner_bits,
                                                                 long long default_bits)
{
    return postgres_dao_groups_modules::NormalizeAdminPermissionBitsForRole(is_admin,
                                                                            permission_bits,
                                                                            owner_bits,
                                                                            default_bits);
}

bool MemoChatTestPostgresDaoGroupsCanModerateOperator(int operator_role)
{
    return postgres_dao_groups_modules::CanModerateOperatorRole(operator_role);
}

bool MemoChatTestPostgresDaoGroupsCanModerateTarget(int operator_role, int target_role)
{
    return postgres_dao_groups_modules::CanModerateTargetRole(operator_role, target_role);
}

long long MemoChatTestPostgresDaoGroupsClampMute(long long mute_until)
{
    return postgres_dao_groups_modules::ClampMuteUntil(mute_until);
}

bool MemoChatTestPostgresDaoGroupsCanQuit(int role)
{
    return postgres_dao_groups_modules::CanQuitGroupRole(role);
}

bool MemoChatTestPostgresDaoGroupsCanDissolve(int role)
{
    return postgres_dao_groups_modules::CanDissolveGroupRole(role);
}

long long MemoChatTestPostgresDaoGroupsFallbackPermForRole(int role, long long owner_bits, long long default_bits)
{
    return postgres_dao_groups_modules::FallbackPermissionBitsForRole(role, owner_bits, default_bits);
}

long long MemoChatTestPostgresDaoGroupsNormalizeStoredPerm(long long stored_bits, long long default_bits)
{
    return postgres_dao_groups_modules::NormalizeStoredPermissionBits(stored_bits, default_bits);
}

bool MemoChatTestPostgresDaoGroupsHasRequiredPerm(long long bits, long long required_bits)
{
    return postgres_dao_groups_modules::HasRequiredPermissionBits(bits, required_bits);
}
