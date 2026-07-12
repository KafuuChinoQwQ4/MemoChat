#include "GateAsyncSideEffects.hpp"

#include "GateAsyncSideEffectDtos.hpp"
#include "ConfigMgr.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"
#include "random/Uuid.hpp"

#include <chrono>
#include <mutex>
#include <utility>

#ifndef MEMOCHAT_ENABLE_KAFKA
#define MEMOCHAT_ENABLE_KAFKA 0
#endif

#if MEMOCHAT_ENABLE_KAFKA
#include <librdkafka/rdkafka.h>
#endif

import memochat.account.async_side_effect_algorithms;

namespace async_algo = memochat::account::async_side_effect::modules;

namespace
{
int64_t NowMsGate()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

#if MEMOCHAT_ENABLE_KAFKA
bool SetKafkaConfig(rd_kafka_conf_t* config, const char* key, const std::string& value, std::string* error)
{
    char error_buffer[512]{};
    if (rd_kafka_conf_set(config, key, value.c_str(), error_buffer, sizeof(error_buffer)) == RD_KAFKA_CONF_OK)
    {
        return true;
    }
    if (error != nullptr)
    {
        *error = error_buffer;
    }
    return false;
}
#endif
} // namespace

GateAsyncSideEffects& GateAsyncSideEffects::Instance()
{
    static GateAsyncSideEffects instance;
    return instance;
}

GateAsyncSideEffects::GateAsyncSideEffects()
{
    auto& cfg = ConfigMgr::Inst();
    _kafka_brokers = cfg.GetValue("Kafka", "Brokers");
    _kafka_client_id = cfg.GetValue("Kafka", "ClientId");
}

GateAsyncSideEffects::~GateAsyncSideEffects()
{
    CloseKafka();
}

void GateAsyncSideEffects::PublishUserProfileChanged(int uid,
                                                     const std::string& user_id,
                                                     const std::string& email,
                                                     const std::string& name,
                                                     const std::string& nick,
                                                     const std::string& icon,
                                                     int sex)
{
    const auto payload =
        gateasync::ToJsonValue(gateasync::BuildUserProfileChangedPayload(uid, user_id, email, name, nick, icon, sex));
    std::string error;
    if (!PublishKafka(async_algo::UserProfileChangedTopic(),
                      std::to_string(uid),
                      async_algo::UserProfileChangedEventType(),
                      payload,
                      &error))
    {
        memolog::LogWarn("gate.side_effect.profile_publish_failed",
                         "failed to publish user profile change",
                         {{"uid", std::to_string(uid)}, {"error", error}});
    }
}

void GateAsyncSideEffects::PublishAuditLogin(int uid,
                                             const std::string& user_id,
                                             const std::string& email,
                                             const std::string& chat_server,
                                             const std::string& chat_host,
                                             const std::string& chat_port,
                                             bool login_cache_hit)
{
    const auto payload = gateasync::ToJsonValue(
        gateasync::BuildLoginAuditPayload(uid, user_id, email, chat_server, chat_host, chat_port, login_cache_hit));
    std::string error;
    if (!PublishKafka(async_algo::AuditLoginTopic(),
                      std::to_string(uid),
                      async_algo::AuditLoginEventType(),
                      payload,
                      &error))
    {
        memolog::LogWarn("gate.side_effect.audit_publish_failed",
                         "failed to publish login audit event",
                         {{"uid", std::to_string(uid)}, {"error", error}});
    }
}

bool GateAsyncSideEffects::PublishKafka(const std::string& topic,
                                        const std::string& partition_key,
                                        const std::string& event_type,
                                        const memochat::json::JsonValue& payload,
                                        std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (_kafka_brokers.empty())
    {
        if (error != nullptr)
        {
            *error = async_algo::KafkaNotConfiguredError();
        }
        return false;
    }
    std::string event_id;
    if (!memochat::random::GenerateUuid(event_id, error))
    {
        return false;
    }
    std::string serialized;
    if (!gateasync::EncodeGateKafkaEventEnvelope(
            gateasync::BuildKafkaEventEnvelope(std::move(event_id),
                                               topic,
                                               partition_key,
                                               event_type,
                                               memolog::TraceContext::GetTraceId(),
                                               memolog::TraceContext::GetRequestId(),
                                               NowMsGate(),
                                               0,
                                               payload),
            &serialized,
            error))
    {
        return false;
    }

    std::lock_guard<std::mutex> guard(_kafka_mutex);
    auto* producer = static_cast<rd_kafka_t*>(_kafka_producer);
    if (producer == nullptr)
    {
        rd_kafka_conf_t* config = rd_kafka_conf_new();
        if (config == nullptr)
        {
            if (error != nullptr)
            {
                *error = "rd_kafka_conf_new: allocation failed";
            }
            return false;
        }
        const std::string client_id = _kafka_client_id.empty() ? async_algo::DefaultKafkaClientId() : _kafka_client_id;
        if (!SetKafkaConfig(config, "bootstrap.servers", _kafka_brokers, error) ||
            !SetKafkaConfig(config, "client.id", client_id, error) ||
            !SetKafkaConfig(config, "socket.timeout.ms", "3000", error) ||
            !SetKafkaConfig(config, "message.timeout.ms", "5000", error) ||
            !SetKafkaConfig(config, "queue.buffering.max.ms", "10", error))
        {
            rd_kafka_conf_destroy(config);
            return false;
        }

        char error_buffer[512]{};
        producer = rd_kafka_new(RD_KAFKA_PRODUCER, config, error_buffer, sizeof(error_buffer));
        if (producer == nullptr)
        {
            rd_kafka_conf_destroy(config);
            if (error != nullptr)
            {
                *error = error_buffer;
            }
            return false;
        }
        _kafka_producer = producer;
    }

    const rd_kafka_resp_err_t produce_error =
        rd_kafka_producev(producer,
                          RD_KAFKA_V_TOPIC(topic.c_str()),
                          RD_KAFKA_V_KEY(const_cast<char*>(partition_key.data()), partition_key.size()),
                          RD_KAFKA_V_VALUE(const_cast<char*>(serialized.data()), serialized.size()),
                          RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                          RD_KAFKA_V_END);
    if (produce_error != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        if (error != nullptr)
        {
            *error = rd_kafka_err2str(produce_error);
        }
        return false;
    }
    rd_kafka_poll(producer, 0);
    return true;
#else
    (void) topic;
    (void) partition_key;
    (void) event_type;
    (void) payload;
    if (error != nullptr)
    {
        *error = async_algo::KafkaBuildDisabledError();
    }
    return false;
#endif
}

void GateAsyncSideEffects::CloseKafka()
{
#if MEMOCHAT_ENABLE_KAFKA
    std::lock_guard<std::mutex> guard(_kafka_mutex);
    if (_kafka_producer != nullptr)
    {
        auto* producer = static_cast<rd_kafka_t*>(_kafka_producer);
        rd_kafka_flush(producer, 1000);
        rd_kafka_destroy(producer);
        _kafka_producer = nullptr;
    }
#endif
}
