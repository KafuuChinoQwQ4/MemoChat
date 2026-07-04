export module memochat.varify.email_task_bus_algorithms;

export namespace memochat::varify::email_task_bus::modules
{
const char* DefaultVHost()
{
    return "/";
}

const char* DefaultDirectExchange()
{
    return "memochat.direct";
}

const char* DefaultDlxExchange()
{
    return "memochat.dlx";
}

const char* DefaultVerifyDeliveryRoutingKey()
{
    return "verify.email.delivery";
}

const char* DefaultVerifyDeliveryQueue()
{
    return "verify.email.delivery.q";
}

const char* DefaultVerifyDeliveryRetryQueue()
{
    return "verify.email.delivery.retry.q";
}

const char* DefaultVerifyDeliveryDlqQueue()
{
    return "verify.email.delivery.dlq.q";
}

const char* DefaultRetryRoutingKey()
{
    return "verify.email.delivery.retry";
}

const char* DefaultDlqRoutingKey()
{
    return "verify.email.delivery.dlq";
}

int DefaultRetryDelayMs()
{
    return 5000;
}

int DefaultMaxRetries()
{
    return 5;
}

int NormalizeRetryDelayMs(int configured_ms)
{
    return configured_ms > 0 ? configured_ms : DefaultRetryDelayMs();
}

int NormalizeMaxRetries(int configured_retries)
{
    return configured_retries > 0 ? configured_retries : DefaultMaxRetries();
}

bool ShouldRetryTask(int retry_count, int max_retries)
{
    return retry_count < max_retries;
}

int NextRetryCount(int retry_count)
{
    return retry_count + 1;
}
} // namespace memochat::varify::email_task_bus::modules
