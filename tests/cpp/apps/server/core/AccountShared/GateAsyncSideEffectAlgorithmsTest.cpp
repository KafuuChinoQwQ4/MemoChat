#include <gtest/gtest.h>

namespace memochat::tests::account::async_side_effect
{
const char* UserProfileChangedTopic();
const char* UserProfileChangedEventType();
const char* AuditLoginTopic();
const char* AuditLoginEventType();
const char* KafkaNotConfiguredError();
const char* KafkaBuildDisabledError();
const char* DefaultKafkaClientId();
} // namespace memochat::tests::account::async_side_effect

TEST(GateAsyncSideEffectAlgorithmsTest, KafkaTopicAndEventLiterals)
{
    using namespace memochat::tests::account::async_side_effect;

    EXPECT_STREQ(UserProfileChangedTopic(), "user.profile.changed.v1");
    EXPECT_STREQ(UserProfileChangedEventType(), "user_profile_changed");
    EXPECT_STREQ(AuditLoginTopic(), "audit.login.v1");
    EXPECT_STREQ(AuditLoginEventType(), "gate_login_succeeded");
    EXPECT_STREQ(DefaultKafkaClientId(), "memochat-gate");
}

TEST(GateAsyncSideEffectAlgorithmsTest, ErrorLiteralsArePinned)
{
    using namespace memochat::tests::account::async_side_effect;

    EXPECT_STREQ(KafkaNotConfiguredError(), "kafka_not_configured");
    EXPECT_STREQ(KafkaBuildDisabledError(), "kafka_build_disabled");
}
