#pragma once

#include <string>

struct RabbitMqConfig {
    std::string host;
    int port = 5672;
    std::string username;
    std::string password;
    std::string vhost = "/";
    int prefetch_count = 50;
    std::string exchange_direct = "memochat.direct";
    std::string exchange_dlx = "memochat.dlx";
    int retry_delay_ms = 5000;
    int max_retries = 5;

    bool valid() const { return !host.empty() && port > 0; }
};

RabbitMqConfig LoadRabbitMqConfig();
