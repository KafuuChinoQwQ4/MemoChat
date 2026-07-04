import memochat.account.async_side_effect_algorithms;

namespace memochat::tests::account::async_side_effect
{
bool ShouldUseIntFallback(bool raw_empty)
{
    return memochat::account::async_side_effect::modules::ShouldUseIntFallback(raw_empty);
}

bool ShouldApplyConfigOverride(bool value_empty)
{
    return memochat::account::async_side_effect::modules::ShouldApplyConfigOverride(value_empty);
}

const char* UserProfileChangedTopic()
{
    return memochat::account::async_side_effect::modules::UserProfileChangedTopic();
}

const char* UserProfileChangedEventType()
{
    return memochat::account::async_side_effect::modules::UserProfileChangedEventType();
}

const char* AuditLoginTopic()
{
    return memochat::account::async_side_effect::modules::AuditLoginTopic();
}

const char* AuditLoginEventType()
{
    return memochat::account::async_side_effect::modules::AuditLoginEventType();
}

const char* CacheInvalidateRoutingKey()
{
    return memochat::account::async_side_effect::modules::CacheInvalidateRoutingKey();
}

const char* CacheInvalidateTaskType()
{
    return memochat::account::async_side_effect::modules::CacheInvalidateTaskType();
}

const char* CacheInvalidateQueue()
{
    return memochat::account::async_side_effect::modules::CacheInvalidateQueue();
}

const char* KafkaNotConfiguredError()
{
    return memochat::account::async_side_effect::modules::KafkaNotConfiguredError();
}

const char* KafkaBuildDisabledError()
{
    return memochat::account::async_side_effect::modules::KafkaBuildDisabledError();
}

const char* RabbitBuildDisabledError()
{
    return memochat::account::async_side_effect::modules::RabbitBuildDisabledError();
}

const char* RabbitNotConfiguredError()
{
    return memochat::account::async_side_effect::modules::RabbitNotConfiguredError();
}

const char* RabbitPublishFailedError()
{
    return memochat::account::async_side_effect::modules::RabbitPublishFailedError();
}

const char* RabbitRpcReplyFailedError()
{
    return memochat::account::async_side_effect::modules::RabbitRpcReplyFailedError();
}

const char* RabbitSocketCreateFailedError()
{
    return memochat::account::async_side_effect::modules::RabbitSocketCreateFailedError();
}

const char* RabbitSocketOpenFailedError()
{
    return memochat::account::async_side_effect::modules::RabbitSocketOpenFailedError();
}

const char* DefaultKafkaClientId()
{
    return memochat::account::async_side_effect::modules::DefaultKafkaClientId();
}
} // namespace memochat::tests::account::async_side_effect
