import memochat.chat.message_service_runtime_algorithms;

#include <string>

namespace message_service_modules = memochat::chat::message_service::modules;

bool MemoChatTestMessageServiceIsConfigFlag(const std::string& value)
{
    return message_service_modules::IsConfigFlag(value.data(), value.size());
}

bool MemoChatTestMessageServiceShouldRejectMissingConfigValue(bool has_next_arg)
{
    return message_service_modules::ShouldRejectMissingConfigValue(has_next_arg);
}

bool MemoChatTestMessageServiceShouldUseDefaultConfigValue(bool value_empty)
{
    return message_service_modules::ShouldUseDefaultConfigValue(value_empty);
}

bool MemoChatTestMessageServiceShouldSetInstanceName(bool instance_name_empty)
{
    return message_service_modules::ShouldSetInstanceName(instance_name_empty);
}

std::string MemoChatTestMessageServiceDefaultName()
{
    return message_service_modules::DefaultServiceName();
}

std::string MemoChatTestMessageServiceDefaultHost()
{
    return message_service_modules::DefaultRpcHost();
}

std::string MemoChatTestMessageServiceDefaultPort()
{
    return message_service_modules::DefaultRpcPort();
}

long long MemoChatTestMessageServiceDefaultDatacenterId()
{
    return message_service_modules::DefaultSnowflakeDatacenterId();
}

long long MemoChatTestMessageServiceDefaultWorkerId()
{
    return message_service_modules::DefaultSnowflakeWorkerId();
}

std::string MemoChatTestMessageServiceDisabledEventPublishError()
{
    return message_service_modules::DisabledEventPublishError();
}

std::string MemoChatTestMessageServiceLoggerName()
{
    return message_service_modules::LoggerName();
}
