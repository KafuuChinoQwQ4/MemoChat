#include <gtest/gtest.h>

namespace memochat::tests::chat::chat_service_impl
{
bool ShouldSkipRecipientSession(bool session_present);
int NextDeliveredCount(int current, bool sent);
int FanoutReplyErrorCode(int success_code);
int PrivateNotifyErrorCode(bool session_present, int success_code, int target_offline_code);
bool ShouldAttachFromUserId(bool has_sender_info, bool user_id_empty);
bool ShouldRefreshFromPostgres(bool user_id_empty);
bool ShouldUseFallbackTimestamp(long long created_at);
} // namespace memochat::tests::chat::chat_service_impl

namespace csi = memochat::tests::chat::chat_service_impl;

TEST(ChatServiceImplAlgorithmsTest, SkipsRecipientWithoutSession)
{
    EXPECT_TRUE(csi::ShouldSkipRecipientSession(false));
    EXPECT_FALSE(csi::ShouldSkipRecipientSession(true));
}

TEST(ChatServiceImplAlgorithmsTest, AdvancesDeliveredCounterOnSend)
{
    EXPECT_EQ(csi::NextDeliveredCount(0, true), 1);
    EXPECT_EQ(csi::NextDeliveredCount(3, true), 4);
    EXPECT_EQ(csi::NextDeliveredCount(3, false), 3);
}

TEST(ChatServiceImplAlgorithmsTest, FanoutReplyAlwaysSuccess)
{
    constexpr int kSuccess = 0;
    EXPECT_EQ(csi::FanoutReplyErrorCode(kSuccess), kSuccess);
}

TEST(ChatServiceImplAlgorithmsTest, PrivateNotifyUsesTargetOfflineWhenNoSession)
{
    constexpr int kSuccess = 0;
    constexpr int kTargetOffline = 1005;
    EXPECT_EQ(csi::PrivateNotifyErrorCode(true, kSuccess, kTargetOffline), kSuccess);
    EXPECT_EQ(csi::PrivateNotifyErrorCode(false, kSuccess, kTargetOffline), kTargetOffline);
}

TEST(ChatServiceImplAlgorithmsTest, AttachesFromUserIdOnlyWhenPresentAndNonEmpty)
{
    EXPECT_TRUE(csi::ShouldAttachFromUserId(true, false));
    EXPECT_FALSE(csi::ShouldAttachFromUserId(true, true));
    EXPECT_FALSE(csi::ShouldAttachFromUserId(false, false));
    EXPECT_FALSE(csi::ShouldAttachFromUserId(false, true));
}

TEST(ChatServiceImplAlgorithmsTest, RefreshesFromPostgresWhenUserIdEmpty)
{
    EXPECT_TRUE(csi::ShouldRefreshFromPostgres(true));
    EXPECT_FALSE(csi::ShouldRefreshFromPostgres(false));
}

TEST(ChatServiceImplAlgorithmsTest, FallsBackTimestampWhenNonPositive)
{
    EXPECT_TRUE(csi::ShouldUseFallbackTimestamp(0));
    EXPECT_TRUE(csi::ShouldUseFallbackTimestamp(-1));
    EXPECT_FALSE(csi::ShouldUseFallbackTimestamp(1));
    EXPECT_FALSE(csi::ShouldUseFallbackTimestamp(1700000000000LL));
}
