#include <gtest/gtest.h>

bool MemoChatTestMessageRepositoryAbortAfterPrimaryWrite(bool primary_write_success);
bool MemoChatTestMessageRepositoryAttemptMongo(bool mongo_enabled);
bool MemoChatTestMessageRepositoryHasLoadedMessage(bool query_success, bool message_present);
bool MemoChatTestMessageRepositoryReturnMongoMessage(bool mongo_success);
bool MemoChatTestMessageRepositoryFallbackToPostgres(bool mongo_success, bool result_empty);
bool MemoChatTestMessageRepositoryMergeReadSuccess(bool mongo_success, bool postgres_success);
bool MemoChatTestMessageRepositoryLogMongoWriteFailure(bool mongo_enabled, bool mongo_write_success);

TEST(MessageRepositoryAlgorithmsTest, AbortsOnlyWhenPrimaryWriteFails)
{
    EXPECT_TRUE(MemoChatTestMessageRepositoryAbortAfterPrimaryWrite(false));
    EXPECT_FALSE(MemoChatTestMessageRepositoryAbortAfterPrimaryWrite(true));
}

TEST(MessageRepositoryAlgorithmsTest, AttemptsMongoOnlyWhenEnabled)
{
    EXPECT_TRUE(MemoChatTestMessageRepositoryAttemptMongo(true));
    EXPECT_FALSE(MemoChatTestMessageRepositoryAttemptMongo(false));
}

TEST(MessageRepositoryAlgorithmsTest, RequiresSuccessfulQueryAndPresentMessage)
{
    EXPECT_TRUE(MemoChatTestMessageRepositoryHasLoadedMessage(true, true));
    EXPECT_FALSE(MemoChatTestMessageRepositoryHasLoadedMessage(true, false));
    EXPECT_FALSE(MemoChatTestMessageRepositoryHasLoadedMessage(false, true));
    EXPECT_FALSE(MemoChatTestMessageRepositoryHasLoadedMessage(false, false));
}

TEST(MessageRepositoryAlgorithmsTest, ReturnsMongoMessageOnlyOnMongoSuccess)
{
    EXPECT_TRUE(MemoChatTestMessageRepositoryReturnMongoMessage(true));
    EXPECT_FALSE(MemoChatTestMessageRepositoryReturnMongoMessage(false));
}

TEST(MessageRepositoryAlgorithmsTest, FallsBackToPostgresOnMongoFailureOrEmptyResult)
{
    EXPECT_FALSE(MemoChatTestMessageRepositoryFallbackToPostgres(true, false));
    EXPECT_TRUE(MemoChatTestMessageRepositoryFallbackToPostgres(true, true));
    EXPECT_TRUE(MemoChatTestMessageRepositoryFallbackToPostgres(false, false));
    EXPECT_TRUE(MemoChatTestMessageRepositoryFallbackToPostgres(false, true));
}

TEST(MessageRepositoryAlgorithmsTest, MergesMongoAndPostgresReadSuccess)
{
    EXPECT_TRUE(MemoChatTestMessageRepositoryMergeReadSuccess(true, false));
    EXPECT_TRUE(MemoChatTestMessageRepositoryMergeReadSuccess(false, true));
    EXPECT_TRUE(MemoChatTestMessageRepositoryMergeReadSuccess(true, true));
    EXPECT_FALSE(MemoChatTestMessageRepositoryMergeReadSuccess(false, false));
}

TEST(MessageRepositoryAlgorithmsTest, LogsMongoWriteFailureOnlyWhenEnabledAndFailed)
{
    EXPECT_TRUE(MemoChatTestMessageRepositoryLogMongoWriteFailure(true, false));
    EXPECT_FALSE(MemoChatTestMessageRepositoryLogMongoWriteFailure(true, true));
    EXPECT_FALSE(MemoChatTestMessageRepositoryLogMongoWriteFailure(false, false));
    EXPECT_FALSE(MemoChatTestMessageRepositoryLogMongoWriteFailure(false, true));
}
