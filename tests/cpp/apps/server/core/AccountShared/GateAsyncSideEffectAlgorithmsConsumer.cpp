import memochat.account.async_side_effect_algorithms;

namespace memochat::tests::account::async_side_effect
{
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

const char* KafkaNotConfiguredError()
{
    return memochat::account::async_side_effect::modules::KafkaNotConfiguredError();
}

const char* KafkaBuildDisabledError()
{
    return memochat::account::async_side_effect::modules::KafkaBuildDisabledError();
}

const char* DefaultKafkaClientId()
{
    return memochat::account::async_side_effect::modules::DefaultKafkaClientId();
}
} // namespace memochat::tests::account::async_side_effect
