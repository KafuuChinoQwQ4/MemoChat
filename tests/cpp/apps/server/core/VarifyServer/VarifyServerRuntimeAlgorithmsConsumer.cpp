import memochat.varify.server_runtime_algorithms;

namespace memochat::tests::varify::server
{
int DefaultHealthPort()
{
    return memochat::varify::server::modules::DefaultHealthPort();
}

const char* HealthPath()
{
    return memochat::varify::server::modules::HealthPath();
}

const char* ReadinessPath()
{
    return memochat::varify::server::modules::ReadinessPath();
}

const char* TextContentType()
{
    return memochat::varify::server::modules::TextContentType();
}

const char* JsonContentType()
{
    return memochat::varify::server::modules::JsonContentType();
}

const char* NotFoundBody()
{
    return memochat::varify::server::modules::NotFoundBody();
}

const char* RedisDownBody()
{
    return memochat::varify::server::modules::RedisDownBody();
}

const char* ReadyBody()
{
    return memochat::varify::server::modules::ReadyBody();
}

const char* HealthBody()
{
    return memochat::varify::server::modules::HealthBody();
}

bool IsHealthPath(const char* target, unsigned long target_size)
{
    return memochat::varify::server::modules::IsHealthPath(target, target_size);
}

bool IsReadinessPath(const char* target, unsigned long target_size)
{
    return memochat::varify::server::modules::IsReadinessPath(target, target_size);
}

bool ShouldReplyNotFound(bool is_health, bool is_ready)
{
    return memochat::varify::server::modules::ShouldReplyNotFound(is_health, is_ready);
}

bool ShouldReportRedisDown(bool is_ready, bool redis_ok)
{
    return memochat::varify::server::modules::ShouldReportRedisDown(is_ready, redis_ok);
}

bool ShouldUseConfiguredHealthPort(bool health_port_text_empty)
{
    return memochat::varify::server::modules::ShouldUseConfiguredHealthPort(health_port_text_empty);
}
} // namespace memochat::tests::varify::server
