#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace Json {
class Value;
}

class GateAsyncSideEffects {
public:
    static GateAsyncSideEffects& Instance();
    void Start();
    void Stop();

    void PublishUserProfileChanged(int uid,
        const std::string& user_id,
        const std::string& email,
        const std::string& name,
        const std::string& nick,
        const std::string& icon,
        int sex);
    void PublishAuditLogin(int uid,
        const std::string& user_id,
        const std::string& email,
        const std::string& chat_server,
        const std::string& chat_host,
        const std::string& chat_port,
        bool login_cache_hit);
    void PublishCacheInvalidate(const std::string& email, const std::string& user_name, const std::string& reason);

private:
    GateAsyncSideEffects();
    ~GateAsyncSideEffects();
    GateAsyncSideEffects(const GateAsyncSideEffects&) = delete;
    GateAsyncSideEffects& operator=(const GateAsyncSideEffects&) = delete;

    bool PublishKafka(const std::string& topic,
        const std::string& partition_key,
        const std::string& event_type,
        const Json::Value& payload,
        std::string* error);
    bool PublishRabbit(const std::string& routing_key,
        const std::string& task_type,
        const Json::Value& payload,
        std::string* error);
    bool EnsureRabbitConnected(std::string* error);
    bool EnsureRabbitTopology(std::string* error);
    void ConsumeCacheInvalidateLoop();
    void HandleCacheInvalidate(const Json::Value& payload);
    void CloseRabbit();
    void CloseKafka();

    std::string _kafka_brokers;
    std::string _kafka_client_id;
    std::shared_ptr<void> _kafka_producer;
    std::mutex _kafka_mutex;
    std::string _rabbit_host;
    int _rabbit_port = 5672;
    std::string _rabbit_username;
    std::string _rabbit_password;
    std::string _rabbit_vhost = "/";
    std::string _rabbit_exchange_direct = "memochat.direct";
    std::string _rabbit_exchange_dlx = "memochat.dlx";
    int _rabbit_prefetch_count = 10;
    void* _rabbit_connection = nullptr;
    std::mutex _mutex;
    std::atomic<bool> _stop{false};
    std::thread _worker;
};
