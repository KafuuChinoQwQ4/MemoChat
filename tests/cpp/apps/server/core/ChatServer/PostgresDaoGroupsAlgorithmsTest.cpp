#include <gtest/gtest.h>

bool MemoChatTestPostgresDaoGroupsCanCreate(int owner_uid, bool name_empty);
int MemoChatTestPostgresDaoGroupsClampLimit(int member_limit);
bool MemoChatTestPostgresDaoGroupsKeepInitialMember(int uid, int owner_uid);
bool MemoChatTestPostgresDaoGroupsWithinLimit(int existing_member_count, int member_limit);
bool MemoChatTestPostgresDaoGroupsHasRoom(int current_member_count, int member_limit);
bool MemoChatTestPostgresDaoGroupsAffectedRows(long long affected_rows);
bool MemoChatTestPostgresDaoGroupsCodeHeader(int length, char prefix, char first_digit);
bool MemoChatTestPostgresDaoGroupsCodeTail(char c);
bool MemoChatTestPostgresDaoGroupsPendingApply(int status);
int MemoChatTestPostgresDaoGroupsReviewedStatus(bool agree);
int MemoChatTestPostgresDaoGroupsJoinSource(bool is_invite);
bool MemoChatTestPostgresDaoGroupsCanManageAdmins(int operator_role);
bool MemoChatTestPostgresDaoGroupsCanTargetBecomeAdmin(int target_role);
bool MemoChatTestPostgresDaoGroupsCanChangeTarget(int operator_role, int target_role);
long long MemoChatTestPostgresDaoGroupsNormalizeAdminPerm(long long permission_bits,
                                                          long long owner_bits,
                                                          long long default_bits);
long long MemoChatTestPostgresDaoGroupsNormalizeAdminPermForRole(bool is_admin,
                                                                 long long permission_bits,
                                                                 long long owner_bits,
                                                                 long long default_bits);
bool MemoChatTestPostgresDaoGroupsCanModerateOperator(int operator_role);
bool MemoChatTestPostgresDaoGroupsCanModerateTarget(int operator_role, int target_role);
long long MemoChatTestPostgresDaoGroupsClampMute(long long mute_until);
bool MemoChatTestPostgresDaoGroupsCanQuit(int role);
bool MemoChatTestPostgresDaoGroupsCanDissolve(int role);
long long MemoChatTestPostgresDaoGroupsFallbackPermForRole(int role, long long owner_bits, long long default_bits);
long long MemoChatTestPostgresDaoGroupsNormalizeStoredPerm(long long stored_bits, long long default_bits);
bool MemoChatTestPostgresDaoGroupsHasRequiredPerm(long long bits, long long required_bits);

TEST(PostgresDaoGroupsAlgorithmsTest, GatesCreateGroupAndMembers)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanCreate(7, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanCreate(0, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanCreate(7, true));

    EXPECT_EQ(2, MemoChatTestPostgresDaoGroupsClampLimit(-10));
    EXPECT_EQ(2, MemoChatTestPostgresDaoGroupsClampLimit(1));
    EXPECT_EQ(120, MemoChatTestPostgresDaoGroupsClampLimit(120));
    EXPECT_EQ(200, MemoChatTestPostgresDaoGroupsClampLimit(201));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsKeepInitialMember(8, 7));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsKeepInitialMember(0, 7));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsKeepInitialMember(7, 7));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsWithinLimit(2, 2));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsWithinLimit(3, 2));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsHasRoom(1, 2));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsHasRoom(2, 2));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsAffectedRows(1));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsAffectedRows(0));
}

TEST(PostgresDaoGroupsAlgorithmsTest, ValidatesGroupCodeShape)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCodeHeader(10, 'g', '1'));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCodeHeader(10, 'g', '9'));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCodeHeader(9, 'g', '1'));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCodeHeader(10, 'u', '1'));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCodeHeader(10, 'g', '0'));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCodeHeader(10, 'g', 'x'));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCodeTail('0'));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCodeTail('9'));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCodeTail('x'));
}

TEST(PostgresDaoGroupsAlgorithmsTest, NormalizesApplyState)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsPendingApply(0));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsPendingApply(1));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsPendingApply(2));

    EXPECT_EQ(1, MemoChatTestPostgresDaoGroupsReviewedStatus(true));
    EXPECT_EQ(2, MemoChatTestPostgresDaoGroupsReviewedStatus(false));
    EXPECT_EQ(1, MemoChatTestPostgresDaoGroupsJoinSource(true));
    EXPECT_EQ(2, MemoChatTestPostgresDaoGroupsJoinSource(false));
}

TEST(PostgresDaoGroupsAlgorithmsTest, GatesAdminRoleChanges)
{
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanManageAdmins(1));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanManageAdmins(2));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanManageAdmins(3));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanTargetBecomeAdmin(1));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanTargetBecomeAdmin(2));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanTargetBecomeAdmin(3));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanChangeTarget(3, 3));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanChangeTarget(2, 1));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanChangeTarget(2, 2));
}

TEST(PostgresDaoGroupsAlgorithmsTest, NormalizesPermissionBits)
{
    constexpr long long kDefaultBits = 0b001011;
    constexpr long long kOwnerBits = 0b111111;

    EXPECT_EQ(kDefaultBits, MemoChatTestPostgresDaoGroupsNormalizeAdminPerm(0, kOwnerBits, kDefaultBits));
    EXPECT_EQ(kDefaultBits, MemoChatTestPostgresDaoGroupsNormalizeAdminPerm(0b1000000, kOwnerBits, kDefaultBits));
    EXPECT_EQ(0b001000, MemoChatTestPostgresDaoGroupsNormalizeAdminPerm(0b001000, kOwnerBits, kDefaultBits));

    EXPECT_EQ(0, MemoChatTestPostgresDaoGroupsNormalizeAdminPermForRole(false, kOwnerBits, kOwnerBits, kDefaultBits));
    EXPECT_EQ(0b001000,
              MemoChatTestPostgresDaoGroupsNormalizeAdminPermForRole(true, 0b001000, kOwnerBits, kDefaultBits));

    EXPECT_EQ(kOwnerBits, MemoChatTestPostgresDaoGroupsFallbackPermForRole(3, kOwnerBits, kDefaultBits));
    EXPECT_EQ(0, MemoChatTestPostgresDaoGroupsFallbackPermForRole(1, kOwnerBits, kDefaultBits));
    EXPECT_EQ(kDefaultBits, MemoChatTestPostgresDaoGroupsFallbackPermForRole(2, kOwnerBits, kDefaultBits));

    EXPECT_EQ(kDefaultBits, MemoChatTestPostgresDaoGroupsNormalizeStoredPerm(0, kDefaultBits));
    EXPECT_EQ(0b010000, MemoChatTestPostgresDaoGroupsNormalizeStoredPerm(0b010000, kDefaultBits));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsHasRequiredPerm(0b0110, 0b0010));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsHasRequiredPerm(0b0100, 0b0011));
}

TEST(PostgresDaoGroupsAlgorithmsTest, GatesModerationAndLifecycle)
{
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanModerateOperator(1));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanModerateOperator(2));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanModerateTarget(3, 2));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanModerateTarget(2, 2));

    EXPECT_EQ(0, MemoChatTestPostgresDaoGroupsClampMute(-5));
    EXPECT_EQ(0, MemoChatTestPostgresDaoGroupsClampMute(0));
    EXPECT_EQ(1000, MemoChatTestPostgresDaoGroupsClampMute(1000));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanQuit(1));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanQuit(2));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanQuit(3));

    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanDissolve(1));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupsCanDissolve(2));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupsCanDissolve(3));
}
