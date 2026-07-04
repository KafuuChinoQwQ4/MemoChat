#include "RabbitMqConfig.hpp"

#include "ConfigMgr.hpp"

import memochat.chat.messaging_config_algorithms;

RabbitMqConfig LoadRabbitMqConfig()
{
    auto& cfg = ConfigMgr::Inst();
    RabbitMqConfig config;
    config.host = cfg.GetValue("RabbitMQ", "Host");
    const auto port = cfg.GetValue("RabbitMQ", "Port");
    config.port = memochat::chat::messaging::modules::ParseIntOr(port.data(), port.size(), config.port);
    config.username = cfg.GetValue("RabbitMQ", "Username");
    config.password = cfg.GetValue("RabbitMQ", "Password");
    const auto vhost = cfg.GetValue("RabbitMQ", "VHost");
    if (!vhost.empty())
    {
        config.vhost = vhost;
    }
    const auto prefetch_count = cfg.GetValue("RabbitMQ", "PrefetchCount");
    config.prefetch_count = memochat::chat::messaging::modules::AtLeast(
        memochat::chat::messaging::modules::ParseIntOr(prefetch_count.data(),
                                                       prefetch_count.size(),
                                                       config.prefetch_count),
        1);
    const auto exchange_direct = cfg.GetValue("RabbitMQ", "ExchangeDirect");
    if (!exchange_direct.empty())
    {
        config.exchange_direct = exchange_direct;
    }
    const auto exchange_dlx = cfg.GetValue("RabbitMQ", "ExchangeDlx");
    if (!exchange_dlx.empty())
    {
        config.exchange_dlx = exchange_dlx;
    }
    const auto retry_delay = cfg.GetValue("RabbitMQ", "RetryDelayMs");
    config.retry_delay_ms = memochat::chat::messaging::modules::AtLeast(
        memochat::chat::messaging::modules::ParseIntOr(retry_delay.data(), retry_delay.size(), config.retry_delay_ms),
        100);
    const auto max_retries = cfg.GetValue("RabbitMQ", "MaxRetries");
    config.max_retries = memochat::chat::messaging::modules::AtLeast(
        memochat::chat::messaging::modules::ParseIntOr(max_retries.data(), max_retries.size(), config.max_retries),
        1);
    return config;
}
