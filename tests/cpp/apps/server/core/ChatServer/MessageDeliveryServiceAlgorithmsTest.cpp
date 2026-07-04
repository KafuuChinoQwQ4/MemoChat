#include <gtest/gtest.h>

namespace memochat::tests::chat::message_delivery_service
{
bool ShouldSkipRecipient(int uid, int exclude_uid);
bool IsDelivered(int error_code, int delivered, int success_code);
bool IsBatchRpcFailure(int error_code, int delivered, int success_code);
bool ShouldSkipFallbackRetry(bool fallback_empty, bool fallback_equals_target);
const char* OfflineNotifyTaskName();
const char* MessageDeliveryRetryTaskName();
const char* OfflineReason();
const char* RpcFailedReason();
const char* InvalidPrivateNotifyReason();
} // namespace memochat::tests::chat::message_delivery_service

namespace mds = memochat::tests::chat::message_delivery_service;

TEST(MessageDeliveryServiceAlgorithmsTest, SkipsNonPositiveAndExcludedRecipients)
{
    EXPECT_TRUE(mds::ShouldSkipRecipient(0, 42));
    EXPECT_TRUE(mds::ShouldSkipRecipient(-1, 42));
    EXPECT_TRUE(mds::ShouldSkipRecipient(42, 42));
    EXPECT_FALSE(mds::ShouldSkipRecipient(7, 42));
}

TEST(MessageDeliveryServiceAlgorithmsTest, DeliveredRequiresSuccessAndPositiveCount)
{
    constexpr int kSuccess = 0;
    EXPECT_TRUE(mds::IsDelivered(kSuccess, 1, kSuccess));
    EXPECT_FALSE(mds::IsDelivered(kSuccess, 0, kSuccess));
    EXPECT_FALSE(mds::IsDelivered(5, 3, kSuccess));
}

TEST(MessageDeliveryServiceAlgorithmsTest, BatchRpcFailureOnErrorOrZeroDelivered)
{
    constexpr int kSuccess = 0;
    EXPECT_FALSE(mds::IsBatchRpcFailure(kSuccess, 1, kSuccess));
    EXPECT_TRUE(mds::IsBatchRpcFailure(kSuccess, 0, kSuccess));
    EXPECT_TRUE(mds::IsBatchRpcFailure(kSuccess, -1, kSuccess));
    EXPECT_TRUE(mds::IsBatchRpcFailure(9, 5, kSuccess));
}

TEST(MessageDeliveryServiceAlgorithmsTest, SkipsFallbackWhenEmptyOrSameServer)
{
    EXPECT_TRUE(mds::ShouldSkipFallbackRetry(true, false));
    EXPECT_TRUE(mds::ShouldSkipFallbackRetry(false, true));
    EXPECT_TRUE(mds::ShouldSkipFallbackRetry(true, true));
    EXPECT_FALSE(mds::ShouldSkipFallbackRetry(false, false));
}

TEST(MessageDeliveryServiceAlgorithmsTest, ExposesStableTaskNamesAndReasons)
{
    EXPECT_STREQ(mds::OfflineNotifyTaskName(), "offline_notify");
    EXPECT_STREQ(mds::MessageDeliveryRetryTaskName(), "message_delivery_retry");
    EXPECT_STREQ(mds::OfflineReason(), "offline");
    EXPECT_STREQ(mds::RpcFailedReason(), "rpc_failed");
    EXPECT_STREQ(mds::InvalidPrivateNotifyReason(), "invalid_private_notify");
}
