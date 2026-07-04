export module memochat.varify.server_runtime_algorithms;

export namespace memochat::varify::server::modules
{
int DefaultHealthPort()
{
    return 8081;
}

const char* HealthPath()
{
    return "/healthz";
}

const char* ReadinessPath()
{
    return "/readyz";
}

const char* TextContentType()
{
    return "text/plain";
}

const char* JsonContentType()
{
    return "application/json";
}

const char* NotFoundBody()
{
    return "not found";
}

const char* RedisDownBody()
{
    return R"({"status":"unhealthy","reason":"redis_down","service":"VarifyServer"})";
}

const char* ReadyBody()
{
    return R"({"status":"ready","service":"VarifyServer"})";
}

const char* HealthBody()
{
    return R"({"status":"ok","service":"VarifyServer"})";
}

bool EqualsAscii(const char* text, unsigned long text_size, const char* expected)
{
    if (text == nullptr || expected == nullptr)
    {
        return false;
    }

    unsigned long index = 0;
    for (; index < text_size; ++index)
    {
        if (expected[index] == '\0' || text[index] != expected[index])
        {
            return false;
        }
    }

    return expected[index] == '\0';
}

bool IsHealthPath(const char* target, unsigned long target_size)
{
    return EqualsAscii(target, target_size, HealthPath());
}

bool IsReadinessPath(const char* target, unsigned long target_size)
{
    return EqualsAscii(target, target_size, ReadinessPath());
}

bool ShouldReplyNotFound(bool is_health, bool is_ready)
{
    return !is_health && !is_ready;
}

bool ShouldReportRedisDown(bool is_ready, bool redis_ok)
{
    return is_ready && !redis_ok;
}

bool ShouldUseConfiguredHealthPort(bool health_port_text_empty)
{
    return !health_port_text_empty;
}
} // namespace memochat::varify::server::modules
