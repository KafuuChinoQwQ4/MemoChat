import memochat.chat.delivery_worker_runtime_algorithms;

#include <string>

namespace delivery_worker_modules = memochat::chat::delivery_worker::modules;

bool MemoChatTestDeliveryWorkerIsConfigFlag(const std::string& value)
{
    return delivery_worker_modules::IsConfigFlag(value.data(), value.size());
}

bool MemoChatTestDeliveryWorkerShouldRejectMissingConfigValue(bool has_next_arg)
{
    return delivery_worker_modules::ShouldRejectMissingConfigValue(has_next_arg);
}

bool MemoChatTestDeliveryWorkerShouldSetInstanceName(bool instance_name_empty)
{
    return delivery_worker_modules::ShouldSetInstanceName(instance_name_empty);
}

bool MemoChatTestDeliveryWorkerShouldRejectEmptyWorkerName(bool worker_name_empty)
{
    return delivery_worker_modules::ShouldRejectEmptyWorkerName(worker_name_empty);
}

std::string MemoChatTestDeliveryWorkerMissingConfigValueMessage()
{
    return delivery_worker_modules::MissingConfigValueMessage();
}

std::string MemoChatTestDeliveryWorkerUnknownArgumentPrefix()
{
    return delivery_worker_modules::UnknownArgumentPrefix();
}

std::string MemoChatTestDeliveryWorkerEmptyWorkerNameMessage()
{
    return delivery_worker_modules::EmptyWorkerNameMessage();
}

long long MemoChatTestDeliveryWorkerDefaultDatacenterId()
{
    return delivery_worker_modules::DefaultSnowflakeDatacenterId();
}

long long MemoChatTestDeliveryWorkerDefaultWorkerId()
{
    return delivery_worker_modules::DefaultSnowflakeWorkerId();
}

std::string MemoChatTestDeliveryWorkerLoggerName()
{
    return delivery_worker_modules::LoggerName();
}

std::string MemoChatTestDeliveryWorkerEnabledText(bool worker_enabled)
{
    return delivery_worker_modules::WorkerEnabledText(worker_enabled);
}
