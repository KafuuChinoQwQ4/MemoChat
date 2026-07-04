export module memochat.account.async_side_effect_algorithms;

export namespace memochat::account::async_side_effect::modules
{
// GateAsyncSideEffects parses optional integer config values, falling back to a
// caller-supplied default when the raw string is empty (empty short-circuits
// before std::stoi is attempted in the normal TU).
bool ShouldUseIntFallback(bool raw_empty)
{
    return raw_empty;
}

// A non-empty config override replaces the built-in default for vhost/exchange
// names.
bool ShouldApplyConfigOverride(bool value_empty)
{
    return !value_empty;
}

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

// RabbitMQ routing-key / task-type literals for the cache-invalidate task.
const char* CacheInvalidateRoutingKey()
{
    return "gate.cache.invalidate";
}

const char* CacheInvalidateTaskType()
{
    return "gate_cache_invalidate";
}

const char* CacheInvalidateQueue()
{
    return "gate.cache.invalidate.q";
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

const char* RabbitBuildDisabledError()
{
    return "rabbitmq_build_disabled";
}

const char* RabbitNotConfiguredError()
{
    return "rabbitmq_not_configured";
}

const char* RabbitPublishFailedError()
{
    return "rabbitmq_publish_failed";
}

const char* RabbitRpcReplyFailedError()
{
    return "amqp_rpc_reply_failed";
}

const char* RabbitSocketCreateFailedError()
{
    return "rabbitmq_socket_create_failed";
}

const char* RabbitSocketOpenFailedError()
{
    return "rabbitmq_socket_open_failed";
}

const char* DefaultKafkaClientId()
{
    return "memochat-gate";
}
} // namespace memochat::account::async_side_effect::modules
