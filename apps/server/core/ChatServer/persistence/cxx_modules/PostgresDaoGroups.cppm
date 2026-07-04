export module memochat.chat.postgres_dao_groups_algorithms;

export namespace memochat::chat::persistence::postgres_dao_groups::modules
{
int MinInt(int left, int right)
{
    return left < right ? left : right;
}

int MaxInt(int left, int right)
{
    return left > right ? left : right;
}

long long MaxLongLong(long long left, long long right)
{
    return left > right ? left : right;
}

bool HasPositiveUid(int uid)
{
    return uid > 0;
}

bool HasPositiveGroupId(long long group_id)
{
    return group_id > 0;
}

bool HasNonEmptyName(bool name_empty)
{
    return !name_empty;
}

bool CanCreateGroup(int owner_uid, bool name_empty)
{
    return HasPositiveUid(owner_uid) && HasNonEmptyName(name_empty);
}

int GroupMemberLimitMin()
{
    return 2;
}

int GroupMemberLimitMax()
{
    return 200;
}

int ClampMemberLimit(int member_limit)
{
    return MaxInt(GroupMemberLimitMin(), MinInt(member_limit, GroupMemberLimitMax()));
}

bool ShouldKeepInitialMember(int uid, int owner_uid)
{
    return HasPositiveUid(uid) && uid != owner_uid;
}

bool IsMemberCountWithinLimit(int existing_member_count, int member_limit)
{
    return existing_member_count <= member_limit;
}

bool HasRoomForMember(int current_member_count, int member_limit)
{
    return current_member_count < member_limit;
}

bool HasAffectedRows(long long affected_rows)
{
    return affected_rows > 0;
}

bool HasValidGeneratedGroupId(long long group_id)
{
    return HasPositiveGroupId(group_id);
}

int GroupCodeLength()
{
    return 10;
}

char GroupCodePrefix()
{
    return 'g';
}

bool IsAsciiDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool IsNonZeroAsciiDigit(char c)
{
    return c >= '1' && c <= '9';
}

bool HasGroupCodeHeader(int length, char prefix, char first_digit)
{
    return length == GroupCodeLength() && prefix == GroupCodePrefix() && IsNonZeroAsciiDigit(first_digit);
}

bool IsGroupCodeTailChar(char c)
{
    return IsAsciiDigit(c);
}

bool IsPendingApplyStatus(int status)
{
    return status == 0;
}

int ReviewedApplyStatus(bool agree)
{
    return agree ? 1 : 2;
}

int JoinSourceForApplyType(bool is_invite)
{
    return is_invite ? 1 : 2;
}

bool CanOperatorManageAdmins(int operator_role)
{
    return operator_role >= 2;
}

bool IsOwnerRole(int role)
{
    return role >= 3;
}

bool IsNormalMemberRole(int role)
{
    return role < 2;
}

bool CanTargetBecomeAdmin(int target_role)
{
    return target_role != 3;
}

bool CanOperatorChangeTargetRole(int operator_role, int target_role)
{
    return operator_role >= 3 || target_role < operator_role;
}

long long NormalizeAdminPermissionBits(long long permission_bits, long long owner_permission_bits, long long default_bits)
{
    if (permission_bits <= 0)
    {
        return default_bits;
    }
    const long long normalized_bits = permission_bits & owner_permission_bits;
    return normalized_bits > 0 ? normalized_bits : default_bits;
}

long long NormalizeAdminPermissionBitsForRole(bool is_admin,
                                              long long permission_bits,
                                              long long owner_permission_bits,
                                              long long default_bits)
{
    return is_admin ? NormalizeAdminPermissionBits(permission_bits, owner_permission_bits, default_bits) : 0;
}

bool CanModerateWithRoleAndPermission(int operator_role, bool has_permission)
{
    return operator_role >= 2 && has_permission;
}

bool CanModerateOperatorRole(int operator_role)
{
    return operator_role >= 2;
}

bool CanModerateTargetRole(int operator_role, int target_role)
{
    return target_role < operator_role;
}

long long ClampMuteUntil(long long mute_until)
{
    return MaxLongLong(0, mute_until);
}

bool CanQuitGroupRole(int role)
{
    return role != 3;
}

bool CanDissolveGroupRole(int role)
{
    return role == 3;
}

long long FallbackPermissionBitsForRole(int role, long long owner_permission_bits, long long default_bits)
{
    if (IsOwnerRole(role))
    {
        return owner_permission_bits;
    }
    if (IsNormalMemberRole(role))
    {
        return 0;
    }
    return default_bits;
}

long long NormalizeStoredPermissionBits(long long stored_bits, long long default_bits)
{
    return stored_bits > 0 ? stored_bits : default_bits;
}

bool HasRequiredPermissionBits(long long bits, long long required_bits)
{
    return (bits & required_bits) == required_bits;
}
} // namespace memochat::chat::persistence::postgres_dao_groups::modules
