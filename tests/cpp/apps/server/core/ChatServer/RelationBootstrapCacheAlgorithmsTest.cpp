#include <gtest/gtest.h>

int MemoChatTestRelationBootstrapCacheSchemaVersion();
bool MemoChatTestRelationBootstrapCacheValidUid(int uid);
int MemoChatTestRelationBootstrapCacheTtlSec(bool has_configured_ttl,
                                             int configured_ttl,
                                             int fallback_ttl,
                                             int minimum_ttl);
bool MemoChatTestRelationBootstrapCacheCurrentSchema(bool payload_is_object, int schema_version);

TEST(RelationBootstrapCacheAlgorithmsTest, KeepsStableCacheSchemaVersion)
{
    EXPECT_EQ(MemoChatTestRelationBootstrapCacheSchemaVersion(), 2);
    EXPECT_TRUE(MemoChatTestRelationBootstrapCacheCurrentSchema(true, 2));
    EXPECT_FALSE(MemoChatTestRelationBootstrapCacheCurrentSchema(true, 1));
    EXPECT_FALSE(MemoChatTestRelationBootstrapCacheCurrentSchema(false, 2));
}

TEST(RelationBootstrapCacheAlgorithmsTest, GuardsInvalidUserIdsBeforeRedisAccess)
{
    EXPECT_FALSE(MemoChatTestRelationBootstrapCacheValidUid(0));
    EXPECT_FALSE(MemoChatTestRelationBootstrapCacheValidUid(-7));
    EXPECT_TRUE(MemoChatTestRelationBootstrapCacheValidUid(1));
}

TEST(RelationBootstrapCacheAlgorithmsTest, SelectsFallbackOrClampedTtl)
{
    EXPECT_EQ(MemoChatTestRelationBootstrapCacheTtlSec(false, 0, 15, 1), 15);
    EXPECT_EQ(MemoChatTestRelationBootstrapCacheTtlSec(true, -3, 15, 1), 1);
    EXPECT_EQ(MemoChatTestRelationBootstrapCacheTtlSec(true, 0, 15, 1), 1);
    EXPECT_EQ(MemoChatTestRelationBootstrapCacheTtlSec(true, 30, 15, 1), 30);
}
