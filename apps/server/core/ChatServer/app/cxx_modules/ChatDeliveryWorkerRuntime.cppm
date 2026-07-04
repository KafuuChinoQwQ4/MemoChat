export module memochat.chat.delivery_worker_runtime_algorithms;

export namespace memochat::chat::delivery_worker::modules
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

bool ShouldSetInstanceName(bool instance_name_empty)
{
    return !instance_name_empty;
}

bool ShouldRejectEmptyWorkerName(bool worker_name_empty)
{
    return worker_name_empty;
}

const char* MissingConfigValueMessage()
{
    return "missing value for --config";
}

const char* UnknownArgumentPrefix()
{
    return "unknown argument: ";
}

const char* EmptyWorkerNameMessage()
{
    return "chat delivery worker node name is empty";
}

long long DefaultSnowflakeDatacenterId()
{
    return 1;
}

long long DefaultSnowflakeWorkerId()
{
    return 1;
}

const char* LoggerName()
{
    return "ChatDeliveryWorker";
}

const char* WorkerEnabledText(bool worker_enabled)
{
    return worker_enabled ? "true" : "false";
}
} // namespace memochat::chat::delivery_worker::modules
