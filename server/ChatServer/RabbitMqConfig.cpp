#include "RabbitMqConfig.h"

#include "ConfigMgr.h"

#include <algorithm>

namespace {
int ParseIntOr(const std::string& raw, int fallback)
{
    if (raw.empty()) {
        return fallback;
    }
    try {
        return std::stoi(raw);
    }
    catch (...) {
        return fallback;
    }
}
}

RabbitMqConfig LoadRabbitMqConfig()
{
    auto& cfg = ConfigMgr::Inst();
    RabbitMqConfig config;
    config.host = cfg.GetValue("RabbitMQ", "Host");
    config.port = ParseIntOr(cfg.GetValue("RabbitMQ", "Port"), config.port);
    config.username = cfg.GetValue("RabbitMQ", "Username");
    config.password = cfg.GetValue("RabbitMQ", "Password");
    const auto vhost = cfg.GetValue("RabbitMQ", "VHost");
    if (!vhost.empty()) {
        config.vhost = vhost;
    }
    config.prefetch_count = std::max(1, ParseIntOr(cfg.GetValue("RabbitMQ", "PrefetchCount"), config.prefetch_count));
    const auto exchange_direct = cfg.GetValue("RabbitMQ", "ExchangeDirect");
    if (!exchange_direct.empty()) {
        config.exchange_direct = exchange_direct;
    }
    const auto exchange_dlx = cfg.GetValue("RabbitMQ", "ExchangeDlx");
    if (!exchange_dlx.empty()) {
        config.exchange_dlx = exchange_dlx;
    }
    config.retry_delay_ms = std::max(100, ParseIntOr(cfg.GetValue("RabbitMQ", "RetryDelayMs"), config.retry_delay_ms));
    config.max_retries = std::max(1, ParseIntOr(cfg.GetValue("RabbitMQ", "MaxRetries"), config.max_retries));
    return config;
}
