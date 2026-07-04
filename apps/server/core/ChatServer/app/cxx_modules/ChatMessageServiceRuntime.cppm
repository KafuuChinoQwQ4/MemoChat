export module memochat.chat.message_service_runtime_algorithms;

export namespace memochat::chat::message_service::modules
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
    return "chatmessageservice1";
}

const char* DefaultRpcHost()
{
    return "127.0.0.1";
}

const char* DefaultRpcPort()
{
    return "50092";
}

long long DefaultSnowflakeDatacenterId()
{
    return 1;
}

long long DefaultSnowflakeWorkerId()
{
    return 10;
}

const char* DisabledEventPublishError()
{
    return "ChatMessageService event publishing is disabled in this scaffold";
}

const char* DisabledEventPublishLogEvent()
{
    return "message_service.event_publish_disabled";
}

const char* DisabledEventPublishLogMessage()
{
    return "ChatMessageService rejected message event publish";
}

const char* LoggerName()
{
    return "ChatMessageService";
}
} // namespace memochat::chat::message_service::modules
