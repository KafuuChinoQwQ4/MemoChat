#include <gtest/gtest.h>

namespace memochat::tests::call::session_math
{
short CallEventNotifyMsgId();
unsigned long long RoomShortIdLength();
long long SessionExpiryMs(long long started_at_ms, int ring_timeout_sec);
bool ShouldComputeDurationSec(long long accepted_at_ms, long long ended_at_ms);
int SessionDurationSec(long long accepted_at_ms, long long ended_at_ms);
} // namespace memochat::tests::call::session_math

namespace
{
using namespace memochat::tests::call::session_math;
} // namespace

TEST(CallSessionMathAlgorithmsTest, PinsCallEventNotifyMsgId)
{
    EXPECT_EQ(CallEventNotifyMsgId(), static_cast<short>(1085));
}

TEST(CallSessionMathAlgorithmsTest, PinsRoomShortIdLength)
{
    EXPECT_EQ(RoomShortIdLength(), 8ULL);
}

TEST(CallSessionMathAlgorithmsTest, SessionExpiryMsAddsRingTimeoutInMilliseconds)
{
    EXPECT_EQ(SessionExpiryMs(0, 45), 45000LL);
    EXPECT_EQ(SessionExpiryMs(1000, 15), 16000LL);
    // int64 promotion before multiply avoids 32-bit overflow for large timeouts.
    EXPECT_EQ(SessionExpiryMs(0, 3000000), 3000000000LL);
}

TEST(CallSessionMathAlgorithmsTest, ShouldComputeDurationRequiresAcceptedThenLaterEnd)
{
    EXPECT_FALSE(ShouldComputeDurationSec(0, 5000));    // never accepted
    EXPECT_FALSE(ShouldComputeDurationSec(5000, 5000)); // ended == accepted
    EXPECT_FALSE(ShouldComputeDurationSec(5000, 4000)); // ended before accepted
    EXPECT_TRUE(ShouldComputeDurationSec(5000, 6000));
}

TEST(CallSessionMathAlgorithmsTest, SessionDurationSecTruncatesToWholeSeconds)
{
    EXPECT_EQ(SessionDurationSec(0, 1000), 1);
    EXPECT_EQ(SessionDurationSec(0, 1999), 1); // truncating division
    EXPECT_EQ(SessionDurationSec(1000, 4500), 3);
}
