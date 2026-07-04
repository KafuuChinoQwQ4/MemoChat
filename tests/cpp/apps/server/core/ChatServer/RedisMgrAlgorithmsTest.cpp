#include <gtest/gtest.h>

int MemoChatTestRedisMgrNormalizeExpireSeconds(int expire_seconds);
bool MemoChatTestRedisMgrStatusOk(int reply_type, const char* reply_text, int status_reply_type);
bool MemoChatTestRedisMgrStringReply(int reply_type, int string_reply_type);
bool MemoChatTestRedisMgrNilReply(int reply_type, int nil_reply_type);
bool MemoChatTestRedisMgrIntegerReply(int reply_type, int integer_reply_type);
bool MemoChatTestRedisMgrPositiveIntegerReply(int reply_type, long long integer_value, int integer_reply_type);
bool MemoChatTestRedisMgrArrayReply(int reply_type, int array_reply_type);
bool MemoChatTestRedisMgrTreatEmptyLockIdentifierAsReleased(bool identifier_empty);

TEST(RedisMgrAlgorithmsTest, NormalizesSetExTtl)
{
    EXPECT_EQ(MemoChatTestRedisMgrNormalizeExpireSeconds(-5), 1);
    EXPECT_EQ(MemoChatTestRedisMgrNormalizeExpireSeconds(0), 1);
    EXPECT_EQ(MemoChatTestRedisMgrNormalizeExpireSeconds(30), 30);
}

TEST(RedisMgrAlgorithmsTest, AcceptsOnlyExactOkStatusReplies)
{
    constexpr int kStatus = 5;
    constexpr int kOther = 6;

    EXPECT_TRUE(MemoChatTestRedisMgrStatusOk(kStatus, "OK", kStatus));
    EXPECT_TRUE(MemoChatTestRedisMgrStatusOk(kStatus, "ok", kStatus));
    EXPECT_FALSE(MemoChatTestRedisMgrStatusOk(kStatus, "Ok", kStatus));
    EXPECT_FALSE(MemoChatTestRedisMgrStatusOk(kStatus, "OK ", kStatus));
    EXPECT_FALSE(MemoChatTestRedisMgrStatusOk(kStatus, nullptr, kStatus));
    EXPECT_FALSE(MemoChatTestRedisMgrStatusOk(kOther, "OK", kStatus));
}

TEST(RedisMgrAlgorithmsTest, ClassifiesReplyTypes)
{
    constexpr int kString = 1;
    constexpr int kArray = 2;
    constexpr int kInteger = 3;
    constexpr int kNil = 4;

    EXPECT_TRUE(MemoChatTestRedisMgrStringReply(kString, kString));
    EXPECT_FALSE(MemoChatTestRedisMgrStringReply(kNil, kString));
    EXPECT_TRUE(MemoChatTestRedisMgrNilReply(kNil, kNil));
    EXPECT_TRUE(MemoChatTestRedisMgrIntegerReply(kInteger, kInteger));
    EXPECT_TRUE(MemoChatTestRedisMgrArrayReply(kArray, kArray));
    EXPECT_FALSE(MemoChatTestRedisMgrArrayReply(kString, kArray));
}

TEST(RedisMgrAlgorithmsTest, RequiresPositiveIntegerWhereCommandNeedsMutation)
{
    constexpr int kInteger = 3;
    constexpr int kOther = 2;

    EXPECT_TRUE(MemoChatTestRedisMgrPositiveIntegerReply(kInteger, 1, kInteger));
    EXPECT_FALSE(MemoChatTestRedisMgrPositiveIntegerReply(kInteger, 0, kInteger));
    EXPECT_FALSE(MemoChatTestRedisMgrPositiveIntegerReply(kInteger, -1, kInteger));
    EXPECT_FALSE(MemoChatTestRedisMgrPositiveIntegerReply(kOther, 1, kInteger));
}

TEST(RedisMgrAlgorithmsTest, PreservesEmptyLockIdentifierReleaseShortcut)
{
    EXPECT_TRUE(MemoChatTestRedisMgrTreatEmptyLockIdentifierAsReleased(true));
    EXPECT_FALSE(MemoChatTestRedisMgrTreatEmptyLockIdentifierAsReleased(false));
}
