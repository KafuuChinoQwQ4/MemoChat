#pragma once

#include <mutex>
#include <string>

#include "json/GlazeCompat.hpp"

class GateAsyncSideEffects
{
public:
    static GateAsyncSideEffects& Instance();

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

private:
    GateAsyncSideEffects();
    ~GateAsyncSideEffects();
    GateAsyncSideEffects(const GateAsyncSideEffects&) = delete;
    GateAsyncSideEffects& operator=(const GateAsyncSideEffects&) = delete;

    bool PublishKafka(const std::string& topic,
                      const std::string& partition_key,
                      const std::string& event_type,
                      const memochat::json::JsonValue& payload,
                      std::string* error);
    void CloseKafka();

    std::string _kafka_brokers;
    std::string _kafka_client_id;
    void* _kafka_producer = nullptr;
    std::mutex _kafka_mutex;
};
