export module memochat.chat.relation_query_service_runtime_algorithms;

export namespace memochat::chat::relation_query_service::modules
{
bool EqualsAscii(const char* data, unsigned long long size, const char* expected, unsigned long long expected_size)
{
    if (data == nullptr || expected == nullptr || size != expected_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        if (data[index] != expected[index])
        {
            return false;
        }
    }
    return true;
}

const char* ConfigFlag()
{
    return "--config";
}

bool IsConfigFlag(const char* data, unsigned long long size)
{
    return EqualsAscii(data, size, ConfigFlag(), 8);
}

bool ShouldRejectMissingConfigValue(bool has_next_arg)
{
    return !has_next_arg;
}

bool ShouldUseDefaultConfigValue(bool value_empty)
{
    return value_empty;
}

bool ShouldSetInstanceName(bool instance_name_empty)
{
    return !instance_name_empty;
}

const char* MissingConfigValueMessage()
{
    return "missing value for --config";
}

const char* UnknownArgumentPrefix()
{
    return "unknown argument: ";
}

const char* DefaultServiceName()
{
    return "chatrelationquery1";
}

const char* DefaultRpcHost()
{
    return "127.0.0.1";
}

const char* DefaultRpcPort()
{
    return "50090";
}

long long DefaultSnowflakeDatacenterId()
{
    return 1;
}

long long DefaultSnowflakeWorkerId()
{
    return 8;
}

const char* LoggerName()
{
    return "ChatRelationQueryService";
}
} // namespace memochat::chat::relation_query_service::modules
