#include <gtest/gtest.h>

namespace memochat::tests::varify::email_task_bus
{
const char* DefaultVHost();
const char* DefaultDirectExchange();
const char* DefaultDlxExchange();
const char* DefaultVerifyDeliveryRoutingKey();
const char* DefaultVerifyDeliveryQueue();
const char* DefaultVerifyDeliveryRetryQueue();
const char* DefaultVerifyDeliveryDlqQueue();
const char* DefaultRetryRoutingKey();
const char* DefaultDlqRoutingKey();
int NormalizeRetryDelayMs(int configured_ms);
int NormalizeMaxRetries(int configured_retries);
bool ShouldRetryTask(int retry_count, int max_retries);
int NextRetryCount(int retry_count);
} // namespace memochat::tests::varify::email_task_bus

TEST(EmailTaskBusAlgorithmsTest, ExposesStableRabbitMqDefaults)
{
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultVHost(), "/");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultDirectExchange(), "memochat.direct");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultDlxExchange(), "memochat.dlx");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultVerifyDeliveryRoutingKey(), "verify.email.delivery");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultVerifyDeliveryQueue(), "verify.email.delivery.q");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultVerifyDeliveryRetryQueue(),
                 "verify.email.delivery.retry.q");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultVerifyDeliveryDlqQueue(),
                 "verify.email.delivery.dlq.q");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultRetryRoutingKey(), "verify.email.delivery.retry");
    EXPECT_STREQ(memochat::tests::varify::email_task_bus::DefaultDlqRoutingKey(), "verify.email.delivery.dlq");
}

TEST(EmailTaskBusAlgorithmsTest, NormalizesRetryBounds)
{
    EXPECT_EQ(memochat::tests::varify::email_task_bus::NormalizeRetryDelayMs(-1), 5000);
    EXPECT_EQ(memochat::tests::varify::email_task_bus::NormalizeRetryDelayMs(0), 5000);
    EXPECT_EQ(memochat::tests::varify::email_task_bus::NormalizeRetryDelayMs(2500), 2500);

    EXPECT_EQ(memochat::tests::varify::email_task_bus::NormalizeMaxRetries(-1), 5);
    EXPECT_EQ(memochat::tests::varify::email_task_bus::NormalizeMaxRetries(0), 5);
    EXPECT_EQ(memochat::tests::varify::email_task_bus::NormalizeMaxRetries(3), 3);
}

TEST(EmailTaskBusAlgorithmsTest, PreservesRetryBoundary)
{
    EXPECT_TRUE(memochat::tests::varify::email_task_bus::ShouldRetryTask(4, 5));
    EXPECT_FALSE(memochat::tests::varify::email_task_bus::ShouldRetryTask(5, 5));
    EXPECT_FALSE(memochat::tests::varify::email_task_bus::ShouldRetryTask(6, 5));
    EXPECT_EQ(memochat::tests::varify::email_task_bus::NextRetryCount(4), 5);
}
