#include <gtest/gtest.h>

bool MemoChatTestOnlineRouteValidUid(int uid);
bool MemoChatTestOnlineRouteCheckRepairSession(int uid, bool has_session);
bool MemoChatTestOnlineRouteUsableServerName(bool server_name_empty);
bool MemoChatTestOnlineRouteReadUserRoute(int uid);
bool MemoChatTestOnlineRouteSearchOnlineSets(int uid);
bool MemoChatTestOnlineRouteUseOnlineSetHit(bool contains_uid);
bool MemoChatTestOnlineRouteWriteUserRoute(int uid, bool server_name_empty);
bool MemoChatTestOnlineRouteClearTrackedRoute(int uid, bool server_name_empty);

TEST(OnlineRouteStoreAlgorithmsTest, GuardsRedisReadsByPositiveUid)
{
    EXPECT_FALSE(MemoChatTestOnlineRouteValidUid(0));
    EXPECT_FALSE(MemoChatTestOnlineRouteReadUserRoute(-1));
    EXPECT_FALSE(MemoChatTestOnlineRouteSearchOnlineSets(0));
    EXPECT_TRUE(MemoChatTestOnlineRouteValidUid(1));
    EXPECT_TRUE(MemoChatTestOnlineRouteReadUserRoute(42));
    EXPECT_TRUE(MemoChatTestOnlineRouteSearchOnlineSets(42));
}

TEST(OnlineRouteStoreAlgorithmsTest, RequiresSessionAndServerNameBeforeRepair)
{
    EXPECT_FALSE(MemoChatTestOnlineRouteCheckRepairSession(0, true));
    EXPECT_FALSE(MemoChatTestOnlineRouteCheckRepairSession(42, false));
    EXPECT_TRUE(MemoChatTestOnlineRouteCheckRepairSession(42, true));

    EXPECT_FALSE(MemoChatTestOnlineRouteUsableServerName(true));
    EXPECT_TRUE(MemoChatTestOnlineRouteUsableServerName(false));
}

TEST(OnlineRouteStoreAlgorithmsTest, GuardsRouteMutationsByUidAndServerName)
{
    EXPECT_FALSE(MemoChatTestOnlineRouteWriteUserRoute(0, false));
    EXPECT_FALSE(MemoChatTestOnlineRouteWriteUserRoute(42, true));
    EXPECT_TRUE(MemoChatTestOnlineRouteWriteUserRoute(42, false));

    EXPECT_FALSE(MemoChatTestOnlineRouteClearTrackedRoute(-1, false));
    EXPECT_FALSE(MemoChatTestOnlineRouteClearTrackedRoute(42, true));
    EXPECT_TRUE(MemoChatTestOnlineRouteClearTrackedRoute(42, false));
}

TEST(OnlineRouteStoreAlgorithmsTest, ReturnsServerOnlyWhenOnlineSetContainsUid)
{
    EXPECT_FALSE(MemoChatTestOnlineRouteUseOnlineSetHit(false));
    EXPECT_TRUE(MemoChatTestOnlineRouteUseOnlineSetHit(true));
}
