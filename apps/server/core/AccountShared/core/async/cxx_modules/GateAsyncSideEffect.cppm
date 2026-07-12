export module memochat.account.async_side_effect_algorithms;

export namespace memochat::account::async_side_effect::modules
{
// Kafka topic / event-type literals for the user profile and login audit
// events published by the gate.
const char* UserProfileChangedTopic()
{
    return "user.profile.changed.v1";
}

const char* UserProfileChangedEventType()
{
    return "user_profile_changed";
}

const char* AuditLoginTopic()
{
    return "audit.login.v1";
}

const char* AuditLoginEventType()
{
    return "gate_login_succeeded";
}

// Stable publish/build error literals reported through the error out-parameter.
const char* KafkaNotConfiguredError()
{
    return "kafka_not_configured";
}

const char* KafkaBuildDisabledError()
{
    return "kafka_build_disabled";
}

const char* DefaultKafkaClientId()
{
    return "memochat-gate";
}
} // namespace memochat::account::async_side_effect::modules
