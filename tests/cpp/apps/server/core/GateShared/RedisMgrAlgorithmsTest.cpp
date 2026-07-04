#include <gtest/gtest.h>

namespace memochat::tests::gate::redis_mgr
{
int DefaultPoolSize();
int NormalizePoolSize(int configured_pool_size);
bool IsReplyType(int reply_type, int expected_type);
bool IsStatusOk(int reply_type, int status_type, const char* status);
bool IsPositiveIntegerReply(int reply_type, int integer_type, long long value);
bool IsNonEmptyStringReply(int reply_type, int string_type, const char* value, unsigned long length);
bool ShouldSkipBatch(unsigned long item_count);
bool IsRedisOk(int status, int ok_status);
bool ShouldCollectPipelineReply(int status, int ok_status, bool has_reply);
} // namespace memochat::tests::gate::redis_mgr

TEST(RedisMgrAlgorithmsTest, NormalizesConnectionPoolSize)
{
    using namespace memochat::tests::gate::redis_mgr;

    EXPECT_EQ(DefaultPoolSize(), 30);
    EXPECT_EQ(NormalizePoolSize(0), 30);
    EXPECT_EQ(NormalizePoolSize(-3), 30);
    EXPECT_EQ(NormalizePoolSize(12), 12);
}

TEST(RedisMgrAlgorithmsTest, ClassifiesStatusAndReplyTypes)
{
    using namespace memochat::tests::gate::redis_mgr;

    constexpr int kStatus = 5;
    constexpr int kInteger = 3;
    constexpr int kString = 1;
    constexpr int kOther = 9;

    EXPECT_TRUE(IsReplyType(kString, kString));
    EXPECT_FALSE(IsReplyType(kOther, kString));
    EXPECT_TRUE(IsStatusOk(kStatus, kStatus, "OK"));
    EXPECT_TRUE(IsStatusOk(kStatus, kStatus, "ok"));
    EXPECT_FALSE(IsStatusOk(kStatus, kStatus, "Ok"));
    EXPECT_FALSE(IsStatusOk(kOther, kStatus, "OK"));
    EXPECT_FALSE(IsStatusOk(kStatus, kStatus, nullptr));
    EXPECT_TRUE(IsPositiveIntegerReply(kInteger, kInteger, 1));
    EXPECT_FALSE(IsPositiveIntegerReply(kInteger, kInteger, 0));
    EXPECT_FALSE(IsPositiveIntegerReply(kOther, kInteger, 1));
    EXPECT_TRUE(IsNonEmptyStringReply(kString, kString, "v", 1));
    EXPECT_FALSE(IsNonEmptyStringReply(kString, kString, "v", 0));
    EXPECT_FALSE(IsNonEmptyStringReply(kString, kString, nullptr, 1));
}

TEST(RedisMgrAlgorithmsTest, ClassifiesBatchAndPipelineGuards)
{
    using namespace memochat::tests::gate::redis_mgr;

    EXPECT_TRUE(ShouldSkipBatch(0));
    EXPECT_FALSE(ShouldSkipBatch(1));
    EXPECT_TRUE(IsRedisOk(0, 0));
    EXPECT_FALSE(IsRedisOk(-1, 0));
    EXPECT_TRUE(ShouldCollectPipelineReply(0, 0, true));
    EXPECT_FALSE(ShouldCollectPipelineReply(0, 0, false));
    EXPECT_FALSE(ShouldCollectPipelineReply(-1, 0, true));
}
