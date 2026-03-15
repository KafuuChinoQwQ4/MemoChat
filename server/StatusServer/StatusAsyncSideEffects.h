#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

namespace Json {
class Value;
}

class StatusAsyncSideEffects {
public:
    explicit StatusAsyncSideEffects(const std::unordered_set<std::string>& known_servers);
    ~StatusAsyncSideEffects();

    void Start();
    void Stop();
    void PublishAuditLogin(int uid,
        const std::string& server_name,
        const std::string& host,
        const std::string& port,
        const std::string& event_type);
    void PublishPresenceRefresh(int uid, const std::string& selected_server, const std::string& reason);

private:
    bool PublishKafka(const std::string& topic,
        const std::string& partition_key,
        const std::string& event_type,
        const Json::Value& payload,
        std::string* error);
    bool PublishRabbit(const std::string& routing_key,
        const std::string& task_type,
        const Json::Value& payload,
        std::string* error);
    void ConsumePresenceRefreshLoop();
    bool EnsureRabbitConnected(std::string* error);
    bool EnsureRabbitTopology(std::string* error);
    void CloseRabbit();
    void HandlePresenceRefresh(const Json::Value& payload);

    std::unordered_set<std::string> _known_servers;
    std::string _kafka_brokers;
    std::string _kafka_client_id;
    std::string _rabbit_host;
    int _rabbit_port = 5672;
    std::string _rabbit_username;
    std::string _rabbit_password;
    std::string _rabbit_vhost = "/";
    std::string _rabbit_exchange_direct = "memochat.direct";
    std::string _rabbit_exchange_dlx = "memochat.dlx";
    int _rabbit_prefetch_count = 10;
    int _rabbit_retry_delay_ms = 5000;
    int _rabbit_max_retries = 5;
    void* _rabbit_connection = nullptr;
    std::mutex _mutex;
    std::atomic<bool> _stop{false};
    std::thread _worker;
};
