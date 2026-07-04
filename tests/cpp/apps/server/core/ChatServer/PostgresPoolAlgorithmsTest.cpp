#include <gtest/gtest.h>

bool MemoChatTestPostgresPoolShouldCreateInitialConnection(int index, int requested_pool_size);
int MemoChatTestPostgresPoolConnectionWaitTimeoutSeconds();
bool MemoChatTestPostgresPoolShouldReconnect(bool stopped);
bool MemoChatTestPostgresPoolShouldWakeForConnection(bool stopped, bool pool_empty);
bool MemoChatTestPostgresPoolShouldReturnNullAfterWait(bool wait_completed, bool stopped);
bool MemoChatTestPostgresPoolHasReturnedConnection(bool has_connection);
bool MemoChatTestPostgresPoolShouldAcceptReturnedConnection(bool stopped);

TEST(PostgresPoolAlgorithmsTest, KeepsInitialConnectionLoopBounds)
{
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldCreateInitialConnection(-1, 3));
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldCreateInitialConnection(0, 3));
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldCreateInitialConnection(2, 3));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldCreateInitialConnection(3, 3));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldCreateInitialConnection(0, 0));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldCreateInitialConnection(0, -1));
}

TEST(PostgresPoolAlgorithmsTest, KeepsWaitTimeoutSeconds)
{
    EXPECT_EQ(MemoChatTestPostgresPoolConnectionWaitTimeoutSeconds(), 5);
}

TEST(PostgresPoolAlgorithmsTest, StopsReconnectWhenClosed)
{
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldReconnect(false));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldReconnect(true));
}

TEST(PostgresPoolAlgorithmsTest, WakesOnlyWhenStoppedOrConnectionAvailable)
{
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldWakeForConnection(true, true));
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldWakeForConnection(false, false));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldWakeForConnection(false, true));
}

TEST(PostgresPoolAlgorithmsTest, ReturnsNullOnTimeoutOrStop)
{
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldReturnNullAfterWait(false, false));
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldReturnNullAfterWait(true, true));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldReturnNullAfterWait(true, false));
}

TEST(PostgresPoolAlgorithmsTest, AcceptsOnlyNonNullReturnedConnectionsWhileOpen)
{
    EXPECT_FALSE(MemoChatTestPostgresPoolHasReturnedConnection(false));
    EXPECT_TRUE(MemoChatTestPostgresPoolHasReturnedConnection(true));
    EXPECT_TRUE(MemoChatTestPostgresPoolShouldAcceptReturnedConnection(false));
    EXPECT_FALSE(MemoChatTestPostgresPoolShouldAcceptReturnedConnection(true));
}
