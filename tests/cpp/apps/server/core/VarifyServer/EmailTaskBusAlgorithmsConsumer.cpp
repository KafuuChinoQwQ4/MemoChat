import memochat.varify.email_task_bus_algorithms;

namespace memochat::tests::varify::email_task_bus
{
const char* DefaultVHost()
{
    return memochat::varify::email_task_bus::modules::DefaultVHost();
}

const char* DefaultDirectExchange()
{
    return memochat::varify::email_task_bus::modules::DefaultDirectExchange();
}

const char* DefaultDlxExchange()
{
    return memochat::varify::email_task_bus::modules::DefaultDlxExchange();
}

const char* DefaultVerifyDeliveryRoutingKey()
{
    return memochat::varify::email_task_bus::modules::DefaultVerifyDeliveryRoutingKey();
}

const char* DefaultVerifyDeliveryQueue()
{
    return memochat::varify::email_task_bus::modules::DefaultVerifyDeliveryQueue();
}

const char* DefaultVerifyDeliveryRetryQueue()
{
    return memochat::varify::email_task_bus::modules::DefaultVerifyDeliveryRetryQueue();
}

const char* DefaultVerifyDeliveryDlqQueue()
{
    return memochat::varify::email_task_bus::modules::DefaultVerifyDeliveryDlqQueue();
}

const char* DefaultRetryRoutingKey()
{
    return memochat::varify::email_task_bus::modules::DefaultRetryRoutingKey();
}

const char* DefaultDlqRoutingKey()
{
    return memochat::varify::email_task_bus::modules::DefaultDlqRoutingKey();
}

int NormalizeRetryDelayMs(int configured_ms)
{
    return memochat::varify::email_task_bus::modules::NormalizeRetryDelayMs(configured_ms);
}

int NormalizeMaxRetries(int configured_retries)
{
    return memochat::varify::email_task_bus::modules::NormalizeMaxRetries(configured_retries);
}

bool ShouldRetryTask(int retry_count, int max_retries)
{
    return memochat::varify::email_task_bus::modules::ShouldRetryTask(retry_count, max_retries);
}

int NextRetryCount(int retry_count)
{
    return memochat::varify::email_task_bus::modules::NextRetryCount(retry_count);
}
} // namespace memochat::tests::varify::email_task_bus
