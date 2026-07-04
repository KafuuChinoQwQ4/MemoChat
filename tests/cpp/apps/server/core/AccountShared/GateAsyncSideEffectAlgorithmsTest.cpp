#include <gtest/gtest.h>

namespace memochat::tests::account::async_side_effect
{
bool ShouldUseIntFallback(bool raw_empty);
bool ShouldApplyConfigOverride(bool value_empty);
const char* UserProfileChangedTopic();
const char* UserProfileChangedEventType();
const char* AuditLoginTopic();
const char* AuditLoginEventType();
const char* CacheInvalidateRoutingKey();
const char* CacheInvalidateTaskType();
const char* CacheInvalidateQueue();
const char* KafkaNotConfiguredError();
const char* KafkaBuildDisabledError();
const char* RabbitBuildDisabledError();
const char* RabbitNotConfiguredError();
const char* RabbitPublishFailedError();
const char* RabbitRpcReplyFailedError();
const char* RabbitSocketCreateFailedError();
const char* RabbitSocketOpenFailedError();
const char* DefaultKafkaClientId();
} // namespace memochat::tests::account::async_side_effect

TEST(GateAsyncSideEffectAlgorithmsTest, ConfigGuardDecisions)
{
    using namespace memochat::tests::account::async_side_effect;

    // Empty raw config -> fall back to default.
    EXPECT_TRUE(ShouldUseIntFallback(true));
    EXPECT_FALSE(ShouldUseIntFallback(false));

    // Non-empty override replaces the built-in default.
    EXPECT_TRUE(ShouldApplyConfigOverride(false));
    EXPECT_FALSE(ShouldApplyConfigOverride(true));
}

TEST(GateAsyncSideEffectAlgorithmsTest, KafkaTopicAndEventLiterals)
{
    using namespace memochat::tests::account::async_side_effect;

    EXPECT_STREQ(UserProfileChangedTopic(), "user.profile.changed.v1");
    EXPECT_STREQ(UserProfileChangedEventType(), "user_profile_changed");
    EXPECT_STREQ(AuditLoginTopic(), "audit.login.v1");
    EXPECT_STREQ(AuditLoginEventType(), "gate_login_succeeded");
    EXPECT_STREQ(DefaultKafkaClientId(), "memochat-gate");
}

TEST(GateAsyncSideEffectAlgorithmsTest, RabbitTaskLiterals)
{
    using namespace memochat::tests::account::async_side_effect;

    EXPECT_STREQ(CacheInvalidateRoutingKey(), "gate.cache.invalidate");
    EXPECT_STREQ(CacheInvalidateTaskType(), "gate_cache_invalidate");
    EXPECT_STREQ(CacheInvalidateQueue(), "gate.cache.invalidate.q");
}

TEST(GateAsyncSideEffectAlgorithmsTest, ErrorLiteralsArePinned)
{
    using namespace memochat::tests::account::async_side_effect;

    EXPECT_STREQ(KafkaNotConfiguredError(), "kafka_not_configured");
    EXPECT_STREQ(KafkaBuildDisabledError(), "kafka_build_disabled");
    EXPECT_STREQ(RabbitBuildDisabledError(), "rabbitmq_build_disabled");
    EXPECT_STREQ(RabbitNotConfiguredError(), "rabbitmq_not_configured");
    EXPECT_STREQ(RabbitPublishFailedError(), "rabbitmq_publish_failed");
    EXPECT_STREQ(RabbitRpcReplyFailedError(), "amqp_rpc_reply_failed");
    EXPECT_STREQ(RabbitSocketCreateFailedError(), "rabbitmq_socket_create_failed");
    EXPECT_STREQ(RabbitSocketOpenFailedError(), "rabbitmq_socket_open_failed");
}
