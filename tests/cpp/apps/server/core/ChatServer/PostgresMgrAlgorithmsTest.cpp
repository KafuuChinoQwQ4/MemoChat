#include <gtest/gtest.h>

bool MemoChatTestPostgresMgrShouldResetDao(bool dao_exists);
bool MemoChatTestPostgresMgrShouldInitializeDao(bool dao_exists);
const char* MemoChatTestPostgresMgrInitFailureEvent();
const char* MemoChatTestPostgresMgrInitFailureMessage();

TEST(PostgresMgrAlgorithmsTest, ResetsOnlyExistingDao)
{
    EXPECT_TRUE(MemoChatTestPostgresMgrShouldResetDao(true));
    EXPECT_FALSE(MemoChatTestPostgresMgrShouldResetDao(false));
}

TEST(PostgresMgrAlgorithmsTest, InitializesOnlyMissingDao)
{
    EXPECT_FALSE(MemoChatTestPostgresMgrShouldInitializeDao(true));
    EXPECT_TRUE(MemoChatTestPostgresMgrShouldInitializeDao(false));
}

TEST(PostgresMgrAlgorithmsTest, KeepsInitFailureLogSurfaceStable)
{
    EXPECT_STREQ("service.postgres_init_failed", MemoChatTestPostgresMgrInitFailureEvent());
    EXPECT_STREQ("PostgresMgr failed to initialize PostgresDao - service cannot start",
                 MemoChatTestPostgresMgrInitFailureMessage());
}
