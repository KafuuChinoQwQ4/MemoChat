#include <gtest/gtest.h>

bool MemoChatTestDistLockAcquireReplyOk(int reply_type, const char* reply_text, int status_reply_type);
bool MemoChatTestDistLockReleaseReplyDeleted(int reply_type, long long integer_value, int integer_reply_type);

TEST(DistLockAlgorithmsTest, AcceptsOnlyStatusOkForAcquire)
{
    constexpr int kStatusReplyType = 5;
    constexpr int kOtherReplyType = 6;

    EXPECT_TRUE(MemoChatTestDistLockAcquireReplyOk(kStatusReplyType, "OK", kStatusReplyType));
    EXPECT_FALSE(MemoChatTestDistLockAcquireReplyOk(kOtherReplyType, "OK", kStatusReplyType));
    EXPECT_FALSE(MemoChatTestDistLockAcquireReplyOk(kStatusReplyType, "ERR", kStatusReplyType));
    EXPECT_FALSE(MemoChatTestDistLockAcquireReplyOk(kStatusReplyType, "OK ", kStatusReplyType));
    EXPECT_FALSE(MemoChatTestDistLockAcquireReplyOk(kStatusReplyType, nullptr, kStatusReplyType));
}

TEST(DistLockAlgorithmsTest, AcceptsOnlyIntegerOneForRelease)
{
    constexpr int kIntegerReplyType = 3;
    constexpr int kOtherReplyType = 4;

    EXPECT_TRUE(MemoChatTestDistLockReleaseReplyDeleted(kIntegerReplyType, 1, kIntegerReplyType));
    EXPECT_FALSE(MemoChatTestDistLockReleaseReplyDeleted(kIntegerReplyType, 0, kIntegerReplyType));
    EXPECT_FALSE(MemoChatTestDistLockReleaseReplyDeleted(kIntegerReplyType, 2, kIntegerReplyType));
    EXPECT_FALSE(MemoChatTestDistLockReleaseReplyDeleted(kOtherReplyType, 1, kIntegerReplyType));
}
