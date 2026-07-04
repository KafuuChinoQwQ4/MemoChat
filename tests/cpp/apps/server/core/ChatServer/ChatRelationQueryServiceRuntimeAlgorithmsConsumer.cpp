import memochat.chat.relation_query_service_runtime_algorithms;

#include <string>

namespace relation_query_service_modules = memochat::chat::relation_query_service::modules;

bool MemoChatTestRelationQueryServiceIsConfigFlag(const std::string& value)
{
    return relation_query_service_modules::IsConfigFlag(value.data(), value.size());
}

bool MemoChatTestRelationQueryServiceShouldRejectMissingConfigValue(bool has_next_arg)
{
    return relation_query_service_modules::ShouldRejectMissingConfigValue(has_next_arg);
}

bool MemoChatTestRelationQueryServiceShouldUseDefaultConfigValue(bool value_empty)
{
    return relation_query_service_modules::ShouldUseDefaultConfigValue(value_empty);
}

bool MemoChatTestRelationQueryServiceShouldSetInstanceName(bool instance_name_empty)
{
    return relation_query_service_modules::ShouldSetInstanceName(instance_name_empty);
}

std::string MemoChatTestRelationQueryServiceDefaultName()
{
    return relation_query_service_modules::DefaultServiceName();
}

std::string MemoChatTestRelationQueryServiceDefaultHost()
{
    return relation_query_service_modules::DefaultRpcHost();
}

std::string MemoChatTestRelationQueryServiceDefaultPort()
{
    return relation_query_service_modules::DefaultRpcPort();
}

long long MemoChatTestRelationQueryServiceDefaultDatacenterId()
{
    return relation_query_service_modules::DefaultSnowflakeDatacenterId();
}

long long MemoChatTestRelationQueryServiceDefaultWorkerId()
{
    return relation_query_service_modules::DefaultSnowflakeWorkerId();
}

std::string MemoChatTestRelationQueryServiceLoggerName()
{
    return relation_query_service_modules::LoggerName();
}
