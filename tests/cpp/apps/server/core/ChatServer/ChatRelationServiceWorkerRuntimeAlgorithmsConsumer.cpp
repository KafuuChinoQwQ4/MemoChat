import memochat.chat.relation_service_worker_runtime_algorithms;

#include <string>

namespace relation_service_worker_modules = memochat::chat::relation_service_worker::modules;

bool MemoChatTestRelationServiceWorkerIsConfigFlag(const std::string& value)
{
    return relation_service_worker_modules::IsConfigFlag(value.data(), value.size());
}

bool MemoChatTestRelationServiceWorkerShouldRejectMissingConfigValue(bool has_next_arg)
{
    return relation_service_worker_modules::ShouldRejectMissingConfigValue(has_next_arg);
}

bool MemoChatTestRelationServiceWorkerShouldUseDefaultConfigValue(bool value_empty)
{
    return relation_service_worker_modules::ShouldUseDefaultConfigValue(value_empty);
}

bool MemoChatTestRelationServiceWorkerShouldSetInstanceName(bool instance_name_empty)
{
    return relation_service_worker_modules::ShouldSetInstanceName(instance_name_empty);
}

std::string MemoChatTestRelationServiceWorkerDefaultName()
{
    return relation_service_worker_modules::DefaultServiceName();
}

std::string MemoChatTestRelationServiceWorkerDefaultHost()
{
    return relation_service_worker_modules::DefaultRpcHost();
}

std::string MemoChatTestRelationServiceWorkerDefaultPort()
{
    return relation_service_worker_modules::DefaultRpcPort();
}

long long MemoChatTestRelationServiceWorkerDefaultDatacenterId()
{
    return relation_service_worker_modules::DefaultSnowflakeDatacenterId();
}

long long MemoChatTestRelationServiceWorkerDefaultWorkerId()
{
    return relation_service_worker_modules::DefaultSnowflakeWorkerId();
}

std::string MemoChatTestRelationServiceWorkerLoggerName()
{
    return relation_service_worker_modules::LoggerName();
}

bool MemoChatTestRelationServiceWorkerIsRabbitMqBackend(const std::string& value)
{
    return relation_service_worker_modules::IsRabbitMqBackend(value.data(), value.size());
}

bool MemoChatTestRelationServiceWorkerIsKafkaBackend(const std::string& value)
{
    return relation_service_worker_modules::IsKafkaBackend(value.data(), value.size());
}

std::string MemoChatTestRelationServiceWorkerEventBusUnavailableError()
{
    return relation_service_worker_modules::EventBusUnavailableError();
}

std::string MemoChatTestRelationServiceWorkerRabbitMqUnavailableLogEvent()
{
    return relation_service_worker_modules::RabbitMqUnavailableLogEvent();
}

std::string MemoChatTestRelationServiceWorkerRabbitMqUnavailableLogMessage()
{
    return relation_service_worker_modules::RabbitMqUnavailableLogMessage();
}

std::string MemoChatTestRelationServiceWorkerKafkaUnavailableLogEvent()
{
    return relation_service_worker_modules::KafkaUnavailableLogEvent();
}

std::string MemoChatTestRelationServiceWorkerKafkaUnavailableLogMessage()
{
    return relation_service_worker_modules::KafkaUnavailableLogMessage();
}
